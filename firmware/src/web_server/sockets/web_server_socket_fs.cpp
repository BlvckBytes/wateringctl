#include "web_server/sockets/web_server_socket_fs.h"

ENUM_LUT_FULL_IMPL(web_server_socket_fs_response, _EVALS_WEB_SERVER_SOCKET_FS_RESPONSE);

static AsyncWebSocket ws(WEB_SERVER_SOCKET_FS_PATH);

/*
============================================================================
                           Shared Request Queue                             
============================================================================
*/

// Task queue ring-buffer
static file_req_task_arg_t task_queue[WEB_SERFER_SOCKET_FS_TASK_QUEUE_LEN];
static size_t task_queue_next_index = 0;

static void task_queue_next(file_req_task_arg_t **req)
{
  *req = &(task_queue[task_queue_next_index++ % WEB_SERFER_SOCKET_FS_TASK_QUEUE_LEN]);
  (*req)->processed = false;
}

/*
============================================================================
                             Shared Utilities                               
============================================================================
*/

static void web_server_socket_fs_respond_progress(
  AsyncWebSocketClient *client,
  int progress
)
{
  scptr char *resp = strfmt_direct("%s;%d", web_server_socket_fs_response_name(WSFS_PROGRESS), progress);
  client->binary(resp);
}

static void web_server_socket_fs_respond_code(
  AsyncWebSocketClient *client,
  web_server_socket_fs_response_t response
)
{
  client->binary(web_server_socket_fs_response_name(response));
}

static void web_server_socket_fs_respond_json(
  AsyncWebSocketClient *client,
  htable_t *jsonh
)
{
  scptr char *jstr = jsonh_stringify(jsonh, 2, 2048);
  client->binary(jstr);
}

INLINED static htable_t *web_server_socket_fs_jsonify_file(File *file)
{
  scptr htable_t *res = htable_make(3, mman_dealloc_nr);

  jsonh_set_bool(res, "isDirectory", file->isDirectory());
  jsonh_set_int(res, "size", (int) file->size());

  scptr char *name = strclone(file->name());
  if (jsonh_set_str(res, "name", (char *) mman_ref(name)) != JOPRES_SUCCESS)
    mman_dealloc(name);

  return (htable_t *) mman_ref(res);
}

INLINED static bool web_server_socket_fs_preproc_existing_file_request(
  AsyncWebSocketClient *client,
  File *target,
  const char *path,
  bool is_directory
)
{
  // Target not available
  *target = SD.open(path);
  if (!*target)
  {
    web_server_socket_fs_respond_code(client, WSFS_TARGET_NOT_EXISTING);
    return false;
  }

  // Requested a directory but is a file
  if (is_directory && !target->isDirectory())
  {
    web_server_socket_fs_respond_code(client, WSFS_NOT_A_DIR);
    return false;
  }

  // Requested a file but is a directory
  if (!is_directory && target->isDirectory())
  {
    web_server_socket_fs_respond_code(client, WSFS_IS_A_DIR);
    return false;
  }

  // All data parsed and the directory flag matches
  return true;
}

/*
============================================================================
                              Command UNTAR                                 
============================================================================
*/

INLINED static char *web_server_socket_fs_join_paths(const char *a, const char *b)
{
  bool a_trailing = a[strlen(a) - 1] == '/';
  bool b_leading = b[0] == '/';

  if (a_trailing && b_leading)
    return strfmt_direct("%s%s", a, &b[1]);

  else if (!a_trailing && !b_leading)
    return strfmt_direct("%s/%s", a, b);
    
  return strfmt_direct("%s%s", a, b);
}

INLINED static void web_server_socket_fs_respond_tar_child(
  AsyncWebSocketClient *client,
  char *file,
  bool is_directory
)
{
  scptr char *header = strfmt_direct(
    "%s;%s;%s",
    web_server_socket_fs_response_name(WSFS_TAR_CHILD_NOT_CREATED),
    file, is_directory ? "true" : "false"
  );
  client->binary(header);
}

static int web_server_socket_fs_proc_untar_header(
  tar_header_translated_t *header,
  void *context_data
)
{
  untar_req_cb_arg *arg = (untar_req_cb_arg *) context_data;
  scptr char *path = web_server_socket_fs_join_paths(arg->containing_dir, header->filename);

  // Remove trailing slash, if applicable
  char *last = &path[strlen(path) - 1];
  if (*last == '/')
    *last = 0;

  // Directory, create directory
  if (header->type == T_DIRECTORY)
  {
    if (!SD.mkdir(path))
    {
      // Send out a collision for this directory
      web_server_socket_fs_respond_tar_child(
        arg->req->client,
        path,
        true
      );

      return 1;
    }

    return 0;
  }

  // Normal, create file
  if (header->type == T_NORMAL)
  {
    // Create file in .tar's directory
    File f = SD.open(path, "w");

    // Could not create file
    if (!f)
    {
      // Send out a collision for this directory
      web_server_socket_fs_respond_tar_child(
        arg->req->client,
        path,
        false
      );

      return 1;
    }

    // Created, store handle for data and end block calls
    *(arg->curr_handle) = f;
    return 0;
  }

  // Don't care about other types, just ignore them and pass
  return 0;
}

static int web_server_socket_fs_proc_untar_write(
  tar_header_translated_t *header,
  void *context_data,
  unsigned char *block,
  int length
)
{
  untar_req_cb_arg *arg = (untar_req_cb_arg *) context_data;

  // No handle created means ignore
  if (!(*(arg->curr_handle)))
    return 0;

  // Write data
  arg->curr_handle->write(block, length);
  return 0;
}

static int web_server_socket_fs_proc_untar_end(
  tar_header_translated_t *header,
  size_t total_bytes_read,
  void *context_data
)
{
  untar_req_cb_arg *arg = (untar_req_cb_arg *) context_data;

  // Close previously opened file, if exists
  if (*(arg->curr_handle))
  {
    arg->curr_handle->close();
    *(arg->curr_handle) = File(NULL);

    // Respond with the current progress
    web_server_socket_fs_respond_progress(
      arg->req->client,
      (total_bytes_read * 100) / arg->tar_file_handle->size()
    );
  }

  return 0;
}

int web_server_socket_fs_proc_untar_read(
  unsigned char *buf,
  size_t bufsize,
  void *context_data
)
{
  untar_req_cb_arg *arg = (untar_req_cb_arg *) context_data;
  return arg->tar_file_handle->read(buf, bufsize);
}

static void web_server_socket_fs_proc_untar_task(void *arg)
{
  file_req_task_arg_t *req = (file_req_task_arg_t *) arg;

  // Create callback struct from routines above
  static tar_callbacks_t callbacks = (tar_callbacks_t) {
    .header_cb = web_server_socket_fs_proc_untar_header,
    .read_cb = web_server_socket_fs_proc_untar_read,
    .write_cb = web_server_socket_fs_proc_untar_write,
    .end_cb = web_server_socket_fs_proc_untar_end
  };

  // Used by the callbacks internally to pass the current file handle around
  // Header: Create, Data: write, End: Close
  File curr_handle = File(NULL);

  char *containing_dir = NULL;

  // Find last slash index in path
  int last_slash_ind = -1;
  for (int i = 0; i < strlen(req->path); i++)
  {
    if (req->path[i] == '/')
      last_slash_ind = i;
  }

  // Assume it's at root
  if (last_slash_ind < 0)
    containing_dir = strfmt_direct("/");

  else
  {
    // Clone and terminate right after the last slash
    containing_dir = strclone(req->path);
    containing_dir[last_slash_ind + 1] = 0;
  }

  // Create callback context argument
  untar_req_cb_arg_t untar_arg = (untar_req_cb_arg_t ) {
    .curr_handle = &curr_handle,
    .tar_file_handle = &(req->file),
    .req = req,
    .containing_dir = containing_dir,
  };

  tar_handle_t tar_handle;
  tar_setup(&callbacks, &untar_arg, &tar_handle);

  // Start reading, the callbacks will take care of writing the contents
  tar_result_t ret = tar_read(&tar_handle);

  // File too short or corrupted in any other way
  if (ret == TR_ERR_READBLOCK)
  {
    web_server_socket_fs_respond_code(req->client, WSFS_TAR_CORRUPTED);
  }

  // Success
  else if (ret == TR_OK)
    web_server_socket_fs_respond_code(req->client, WSFS_UNTARED);

  // Internal error (don't go into too much detail, as the client won't care)
  // And not a callback error, as the callback itself will respond with the proper error code
  else if (
    ret != TR_ERR_HEADERCB ||
    ret != TR_ERR_WRITECB ||
    ret != TR_ERR_ENDCB
  ) {
    web_server_socket_fs_respond_code(req->client, WSFS_TAR_INTERNAL);
  }

  mman_dealloc(containing_dir);
}

static void web_server_socket_fs_proc_untar(
  AsyncWebSocketClient *client,
  char *path
)
{
  // Open target tar file
  File file = SD.open(path, "r");
  if (!file)
  {
    web_server_socket_fs_respond_code(client, WSFS_TARGET_NOT_EXISTING);
    return;
  }

  // Enqueue untar task
  file_req_task_arg_t *req;
  task_queue_next(&req);
  req->client = client;
  req->path = strclone(path);
  req->file = file;
  req->type = FRT_UNTAR;
}

/*
============================================================================
                              Command FETCH                                 
============================================================================
*/

static void web_server_socket_fs_proc_fetch_task(void *arg)
{
  file_req_task_arg_t *req = (file_req_task_arg_t *) arg;
  File target = SD.open(req->path);

  // Transmit header first
  char *header = strfmt_direct(
    "%s;%lu",
    web_server_socket_fs_response_name(WSFS_FILE_FOUND),
    target.size()
  );
  req->client->binary(header);
  mman_dealloc(header);

  // Now transmit file in chunks
  uint8_t read_buf[4096];

  // Delay between read/send iterations
  const TickType_t xDelay = 2 / portTICK_PERIOD_MS;
  vTaskDelay(xDelay);

  size_t read;
  while ((read = target.read(read_buf, sizeof(read_buf))) > 0)
  {
    // Wait for queue to empty out before an overflow occurs
    while (req->client->queueIsFull());
    req->client->binary(read_buf, read);
    vTaskDelay(xDelay);
  }

  // Done transmitting
  target.close();
}

static void web_server_socket_fs_proc_fetch(
  AsyncWebSocketClient *client,
  char *path,
  bool is_directory
)
{
  File target;
  if (!web_server_socket_fs_preproc_existing_file_request(client, &target, path, is_directory))
    return;

  // Requested a directory, list files
  if (is_directory)
  {
    // Create basic response structure
    scptr htable_t *res = htable_make(1, mman_dealloc_nr);
    scptr dynarr_t *items = dynarr_make(16, 1024, mman_dealloc_nr);
    jsonh_set_arr(res, "items", items);

    // Loop all available files and jsonify them into the items array

    File curr_file;
    while (curr_file = target.openNextFile())
    {
      scptr htable_t *f_jsn = web_server_socket_fs_jsonify_file(&curr_file);
      if (jsonh_insert_arr_obj(items, (htable_t *) mman_ref(f_jsn)) != JOPRES_SUCCESS)
        mman_dealloc(f_jsn);
    }

    web_server_socket_fs_respond_json(client, res);
    target.close();
    return;
  }

  // Not needed anymore
  target.close();

  // Enqueue list task
  file_req_task_arg_t *req;
  task_queue_next(&req);
  req->client = client;
  req->path = strclone(path);
  req->type = FRT_FETCH_LIST;
}

/*
============================================================================
                              Command DELETE                                
============================================================================
*/

static void web_server_socket_fs_recursive_delete(const char *path, bool *success)
{
  // Unsuccessful sub-op
  if (!(*success))
    return;

  // Check if file exists
  File targ = SD.open(path);
  if (!targ)
  {
    *success = false;
    return;
  }

  // Check if file is a directory
  if (targ.isDirectory())
  {
    // Delete all children
    File child;
    while (child = targ.openNextFile())
    {
      // Unsuccessful sub-op
      if (!(*success))
        break;

      web_server_socket_fs_recursive_delete(child.name(), success);
    }

    // Delete the directory itself
    if (*success)
    {
      if (!SD.rmdir(path))
        *success = false;
    }

    targ.close();
    return;
  }

  // Just a file, delete it
  if (!SD.remove(path))
    *success = false;

  targ.close();
}

static void web_server_socket_fs_recursive_delete_task(void *arg)
{
  file_req_task_arg_t *req = (file_req_task_arg_t *) arg;

  bool success = true;
  web_server_socket_fs_recursive_delete(req->path, &success);

  if (!success)
    web_server_socket_fs_respond_code(req->client, WSFS_COULD_NOT_DELETE_DIR);
  else
    web_server_socket_fs_respond_code(req->client, WSFS_DELETED);
}

static void web_server_socket_fs_proc_delete(
  AsyncWebSocketClient *client,
  char *path,
  bool is_directory
)
{
  File target;
  if (!web_server_socket_fs_preproc_existing_file_request(client, &target, path, is_directory))
    return;

  // Requested a directory, delete recursively
  if (is_directory)
  {
    // Enqueue delete task
    file_req_task_arg_t *req;
    task_queue_next(&req);
    req->client = client;
    req->path = strclone(path);
    req->type = FRT_DELETE_DIR;
    return;
  }

  // Requested a file, delete the file
  if (!SD.remove(path))
  {
    web_server_socket_fs_respond_code(client, WSFS_COULD_NOT_DELETE_FILE);
    return;
  }

  web_server_socket_fs_respond_code(client, WSFS_DELETED);
}

/*
============================================================================
                              Command WRITE                                 
============================================================================
*/

/**
 * @brief Process a WRITE command
 * 
 * @return NULL if command is done, a file to be used to write remaining frames and segments otherwise
 */
static File web_server_socket_fs_proc_write(
  AsyncWebSocketClient *client,
  char *path,
  bool is_directory,
  bool overwrite
)
{
  // Already exists
  File target = SD.open(path);
  if (target)
  {
    // Not in overwrite mode
    if (!overwrite)
    {
      web_server_socket_fs_respond_code(
        client,
        target.isDirectory() ? WSFS_DIR_EXISTS : WSFS_FILE_EXISTS
      );

      target.close();
      return File(NULL);
    }

    target.close();
  }

  // Create directory if not exists
  if (is_directory)
  {
    // Could not create directory
    if (!SD.mkdir(path))
    {
      web_server_socket_fs_respond_code(client, WSFS_COULD_NOT_CREATE_DIR);
      return File(NULL);
    }

    // Created directory
    web_server_socket_fs_respond_code(client, WSFS_DIR_CREATED);
    return File(NULL);
  }

  // Create new file
  target = SD.open(path, "w");
  if (!target)
  {
    web_server_socket_fs_respond_code(client, WSFS_COULD_NOT_CREATE_FILE);
    return File(NULL);
  }

  // Return file for further write calls
  web_server_socket_fs_respond_code(client, WSFS_FILE_CREATED);
  return target;
}

/*
============================================================================
                              Command UPDATE                                
============================================================================
*/

static void web_server_socket_fs_proc_update(
  AsyncWebSocketClient *client,
  char *path
)
{
  // Needs to end in .bin
  char *dot = strrchr(path, '.');
  if (!dot || !strcmp(dot, ".bin"))
  {
    web_server_socket_fs_respond_code(client, WSFS_NOT_A_BIN);
    return;
  }

  // Open target binary file
  File file = SD.open(path, "r");
  if (!file)
  {
    web_server_socket_fs_respond_code(client, WSFS_TARGET_NOT_EXISTING);
    return;
  }

  // Cannot flash from dir
  if (file.isDirectory())
  {
    file.close();
    web_server_socket_fs_respond_code(client, WSFS_NOT_A_BIN);
    return;
  }

  // Enqueue update task
  file_req_task_arg_t *req;
  task_queue_next(&req);
  req->client = client;
  req->path = strclone(path);
  req->file = file;
  req->type = FRT_UPDATE;
}

static void web_server_socket_fs_proc_update_task(void *arg)
{
  file_req_task_arg_t *req = (file_req_task_arg_t *) arg;

  // Initialize update
  if (!Update.begin(req->file.size()))
  {
    web_server_socket_fs_respond_code(req->client, WSFS_UPDATE_FAILED);
    return;
  }

  Update.onProgress([req](size_t read, size_t total) {
    web_server_socket_fs_respond_progress(req->client, read * 100 / total);
  });

  // Write whole file
  Update.writeStream(req->file);

  // Update successful
  if (Update.end())
  {
    web_server_socket_fs_respond_code(req->client, WSFS_UPDATED);

    // Wait for the socket send buffer to be able to empty out
    vTaskDelay(5 / portTICK_PERIOD_MS);

    // Restart the system
    ESP.restart();
    return;
  }

  // Update failed
  web_server_socket_fs_respond_code(req->client, WSFS_UPDATE_FAILED);
}

/*
============================================================================
                              Request Router                                
============================================================================
*/

// "Further writes"-file information (file writes are chunked)
static File w_f = File(NULL);
static long w_f_sz = 0;
static unsigned long w_f_last = 0;

INLINED static void w_f_reset()
{
  w_f.close();
  w_f_sz = 0;
  w_f_last = 0;
}

static void web_server_socket_fs_handle_data(
  AsyncWebSocketClient *client,
  AwsFrameInfo *finf,
  uint8_t *data,
  size_t len
)
{
  // Ignore empty requests
  if (len == 0)
    return;

  // Never accept non-binary data
  if (!(
    finf->opcode == WS_BINARY ||
    finf->opcode == WS_CONTINUATION
  ))
  {
    web_server_socket_fs_respond_code(client, WSFS_NON_BINARY_DATA);
    return;
  }

  // Active write
  if (w_f)
  {
    // Write timed out, reset and stop now
    if (w_f_last != 0 && millis() - w_f_last > WEB_SERFER_SOCKET_FS_WRITE_TIMEOUT)
    {
      w_f_reset();
    }

    // Still within timing requirements
    else {
      // Write full data
      w_f.write(data, len);
      w_f_last = millis();

      // Check for completion
      w_f_sz -= len;
      if (w_f_sz == 0)
        w_f_reset();

      web_server_socket_fs_respond_code(client, WSFS_FILE_APPENDED);
      return;
    }
  }

  // First pice of data, this contains all parameters that decide further processing
  if (finf->index == 0 && finf->opcode != WS_CONTINUATION)
  {
    // Parse ASCII parameters from binary data
    size_t data_offs = 0;
    scptr char *cmd = partial_strdup((char *) data, &data_offs, ";", false);
    scptr char *path = partial_strdup((char *) data, &data_offs, ";", false);
    scptr char *is_directory = partial_strdup((char *) data, &data_offs, ";", false);

    if (!cmd || !path)
    {
      web_server_socket_fs_respond_code(client, WSFS_PARAM_MISSING);
      return;
    }

    if (strncasecmp("untar", cmd, strlen("untar")) == 0)
    {
      web_server_socket_fs_proc_untar(client, path);
      return;
    }

    if (strncasecmp("update", cmd, strlen("update")) == 0)
    {
      web_server_socket_fs_proc_update(client, path);
      return;
    }

    if (!is_directory)
    {
      web_server_socket_fs_respond_code(client, WSFS_PARAM_MISSING);
      return;
    }

    // Parse boolean from string
    bool is_directory_bool = strncasecmp("true", is_directory, strlen("true")) == 0;

    if (strncasecmp("fetch", cmd, strlen("fetch")) == 0)
    {
      web_server_socket_fs_proc_fetch(client, path, is_directory_bool);
      return;
    }

    if (strncasecmp("delete", cmd, strlen("delete")) == 0)
    {
      web_server_socket_fs_proc_delete(client, path, is_directory_bool);
      return;
    }

    bool is_overwrite = strncasecmp("overwrite", cmd, strlen("overwrite")) == 0;
    if (strncasecmp("write", cmd, strlen("write")) == 0 || is_overwrite)
    {
      // Parse upload size from params
      scptr char *upload_size = partial_strdup((char *) data, &data_offs, ";", false);
      if (upload_size != NULL)
        longp(&w_f_sz, upload_size, 10);

      w_f = web_server_socket_fs_proc_write(
        client,
        path,
        is_directory_bool,
        is_overwrite
      );

      return;
    }
  
    web_server_socket_fs_respond_code(client, WSFS_COMMAND_UNKNOWN);
    return;
  }
}

/*
============================================================================
                              Socket Events                                 
============================================================================
*/

static void onEvent(
  AsyncWebSocket *server,
  AsyncWebSocketClient *client,
  AwsEventType type,
  void *arg,
  uint8_t *data,
  size_t len
) {
  switch (type) {
    case WS_EVT_DATA:
    {
      web_server_socket_fs_handle_data(client, (AwsFrameInfo*) arg, data, len);
      break;
    }

    case WS_EVT_DISCONNECT:
    case WS_EVT_CONNECT:
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

/*
============================================================================
                           Shared Request Queue                             
============================================================================
*/

static void task_queue_reset_task(file_req_task_arg_t *task)
{
  // Task has now been processed
  task->processed = true;

  // Close file if a type that uses it occurred
  if (task->type == FRT_UNTAR)
    task->file.close();

  // Dealloc path string
  mman_dealloc(task->path);
}

static void task_queue_worker_task(void *arg)
{
  while (true)
  {
    // Check all tasks in the ringbuffer
    for (size_t i = 0; i < WEB_SERFER_SOCKET_FS_TASK_QUEUE_LEN; i++)
    {
      file_req_task_arg_t *curr_task = &(task_queue[i]);

      // Already processed, skip
      if (curr_task->processed)
        continue;

      // Invoke handler function
      switch (curr_task->type)
      {
      case FRT_UNTAR:
        web_server_socket_fs_proc_untar_task(curr_task);
        break;

      case FRT_DELETE_DIR:
        web_server_socket_fs_recursive_delete_task(curr_task);
        break;
      
      case FRT_FETCH_LIST:
        web_server_socket_fs_proc_fetch_task(curr_task);
        break;

      case FRT_UPDATE:
        web_server_socket_fs_proc_update_task(curr_task);
        break;

      // Task unknown
      default:
        break;
      }

      // Task has now been processed, reset it
      task_queue_reset_task(curr_task);
    }

    // Delay for 5ms between scans
    vTaskDelay(5 / portTICK_PERIOD_MS);
  }
}

static void task_queue_init()
{
  // All tasks start out as cleared
  for (size_t i = 0; i < WEB_SERFER_SOCKET_FS_TASK_QUEUE_LEN; i++)
    task_queue_reset_task(&(task_queue[i]));

  // Invoke the task on core 1 (since 0 handles the kernel-protocol)
  xTaskCreatePinnedToCore(
    task_queue_worker_task,                       // Task entry point
    "fs_worker",                                  // Task name
    16384,                                        // Stack size (should be sufficient, I hope)
    NULL,                                         // Parameter to the entry point
    WEB_SERFER_SOCKET_FS_CMD_TASK_PRIO,           // Priority
    NULL,                                         // Task handle output, don't care
    1                                             // On core 1 (main loop)
  );
}

/*
============================================================================
                              Initialization                                
============================================================================
*/

void web_server_socket_fs_init(AsyncWebServer *wsrv)
{
  ws.onEvent(onEvent);
  wsrv->addHandler(&ws);
  task_queue_init();

  dbginf("Started the websocket server for " WEB_SERVER_SOCKET_FS_PATH "!");
}

void web_server_socket_fs_cleanup()
{
  ws.cleanupClients();
}