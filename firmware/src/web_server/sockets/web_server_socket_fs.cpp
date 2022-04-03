#include "web_server/sockets/web_server_socket_fs.h"

ENUM_LUT_FULL_IMPL(web_server_socket_fs_response, _EVALS_WEB_SERVER_SOCKET_FS_RESPONSE);

static AsyncWebSocket ws(WEB_SERVER_SOCKET_FS_PATH);

/*
============================================================================
                             Shared Utilities                               
============================================================================
*/

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
  scptr char *jstr = jsonh_stringify(jsonh, 2);
  client->binary(jstr);
}

INLINED static htable_t *web_server_socket_fs_jsonify_file(File *file)
{
  scptr htable_t *res = jsonh_make();

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
                              Command FETCH                                 
============================================================================
*/

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
    scptr htable_t *res = jsonh_make();
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

  // Requested a file, send file contents
  client->binary(target.readString());
  target.close();
}

/*
============================================================================
                              Command DELETE                                
============================================================================
*/

static void web_server_socket_fs_rec_fdel_req_cleanup(mman_meta_t *meta)
{
  rec_fdel_req *req = (rec_fdel_req *) meta->ptr;
  mman_dealloc(req->path);
}

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
  rec_fdel_req_t *req = (rec_fdel_req_t *) arg;

  bool success = true;
  web_server_socket_fs_recursive_delete(req->path, &success);

  if (!success)
    web_server_socket_fs_respond_code(req->client, WSFS_COULD_NOT_DELETE_DIR);
  else
    web_server_socket_fs_respond_code(req->client, WSFS_DELETED);

  mman_dealloc(req);
  vTaskDelete(NULL);
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
    // Create a new file deletion request wrapper struct instance, to be passed as an argument to the task
    scptr rec_fdel_req_t *req = (rec_fdel_req_t *) mman_alloc(sizeof(rec_fdel_req_t), 1, web_server_socket_fs_rec_fdel_req_cleanup);
    req->client = client;
    req->path = strclone(path);

    // Invoke the task on core 1 (since 0 handles the kernel-protocol)
    xTaskCreatePinnedToCore(
      web_server_socket_fs_recursive_delete_task,   // Task entry point
      "rec_fdel",                                   // Task name
      16384,                                        // Stack size (should be sufficient, I hope)
      mman_ref(req),                                // Parameter to the entry point
      configMAX_PRIORITIES - 1,                     // Priority, as high as possible
      NULL,                                         // Task handle output, don't care
      1                                             // On core 1 (main loop)
    );
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

static void web_server_socket_fs_proc_write(
  AsyncWebSocketClient *client,
  char *path,
  bool is_directory
)
{
  // TODO: Implement
}

/*
============================================================================
                              Request Router                                
============================================================================
*/

static void web_server_socket_fs_handle_data(
  AsyncWebSocketClient *client,
  AwsFrameInfo *finf,
  uint8_t *data,
  size_t len
)
{
  // Never accept non-binary data
  if (finf->opcode != WS_BINARY)
  {
    web_server_socket_fs_respond_code(client, WSFS_NON_BINARY_DATA);
    return;
  }

  // Never accept empty requests
  if (finf->len == 0)
  {
    web_server_socket_fs_respond_code(client, WSFS_EMPTY_REQUEST);
    return;
  }

  // First packet, this contains all parameters that decide processing
  if (finf->index == 0)
  {
    // Parse ASCII parameters from binary data
    size_t data_offs = 0;
    scptr char *cmd = partial_strdup((char *) data, &data_offs, ";", false);
    scptr char *path = partial_strdup((char *) data, &data_offs, ";", false);
    scptr char *is_directory = partial_strdup((char *) data, &data_offs, ";", false);

    // All parameters need to be provided
    if (!cmd || !path || !is_directory)
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

    if (strncasecmp("write", cmd, strlen("write")) == 0)
    {
      web_server_socket_fs_proc_write(client, path, is_directory_bool);
      return;
    }
  
    web_server_socket_fs_respond_code(client, WSFS_COMMAND_UNKNOWN);
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

    /*
      Command protocol:

      req: FETCH;<path>;<is_directory>
      res: json file listing | file content

      req: DELETE;<path>;<is_directory>
      res: ERROR_NOT_FOUND | SUCCESS_DELETED

      req: WRITE;<path>;<is_directory>;[file_bytes]
      res: ERROR_NOT_FOUND | SUCCESS_WRITTEN

      NOTE: writing should maybe be cut-through to also make huge files handleable
    */
    case WS_EVT_DATA:
    {
      web_server_socket_fs_handle_data(client, (AwsFrameInfo*) arg, data, len);
      break;
    }

    case WS_EVT_CONNECT:
    case WS_EVT_DISCONNECT:
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
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
  dbginf("Started the websocket server for " WEB_SERVER_SOCKET_FS_PATH "!");
}

void web_server_socket_fs_cleanup()
{
  ws.cleanupClients();
}