#include "web_server/routes/web_server_route_fs.h"

/*
============================================================================
                                 Routines                                   
============================================================================
*/

INLINED static bool web_server_parse_query_path(AsyncWebServerRequest *request, const char **path)
{
  AsyncWebParameter *param = request->getParam("path");

  if (!param)
  {
    web_server_error_resp(request, 400, QUERY_PARAM_MISSING, "Query parameter missing: " QUOTSTR, "path");
    return false;
  }

  *path = param->value().c_str();
  return true;
}

INLINED static void web_server_parse_query_is_directory(AsyncWebServerRequest *request, bool *is_directory)
{
  AsyncWebParameter *param = request->getParam("isDirectory");
  *is_directory = param ? param->value().equals("true") : false;
}

INLINED static htable_t *web_server_jsonify_file(File *file)
{
  scptr htable_t *res = jsonh_make();

  jsonh_set_bool(res, "isDirectory", file->isDirectory());
  jsonh_set_int(res, "size", (int) file->size());

  scptr char *name = strclone(file->name());
  if (jsonh_set_str(res, "name", (char *) mman_ref(name)) != JOPRES_SUCCESS)
    mman_dealloc(name);

  return (htable_t *) mman_ref(res);
}

INLINED static bool web_server_parse_file_request(
  AsyncWebServerRequest *request,
  File *target,
  const char **path,
  bool *is_directory
)
{
  // Parse path from query
  if (!web_server_parse_query_path(request, path))
    return false;

  // Parse isDirectory flag from query
  web_server_parse_query_is_directory(request, is_directory);

  // Target not available
  *target = SD.open(*path);
  if (!*target)
  {
    web_server_error_resp(
      request, 404, *is_directory ? DIR_NOT_EXISTING : FILE_NOT_EXISTING,
      "The target " QUOTSTR " does not exist", *path
    );
    return false;
  }

  // Requested a directory but is a file
  if (*is_directory && !target->isDirectory())
  {
    web_server_error_resp(request, 400, NOT_A_DIR, "The file " QUOTSTR " is not a directory", *path);
    return false;
  }

  // Requested a file but is a directory
  if (!(*is_directory) && target->isDirectory())
  {
    web_server_error_resp(request, 400, IS_A_DIR, "The file " QUOTSTR " is a directory", *path);
    return false;
  }

  // All data parsed and the directory flag matches
  return true;
}

static void web_server_recursive_delete(const char *path, bool *success)
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

      web_server_recursive_delete(child.name(), success);
    }

    // Delete the directory itself
    if (*success)
    {
      dbginf("deleting dir %s", path);

      if (!SD.rmdir(path))
        *success = false;
    }

    targ.close();
    return;
  }

  // Just a file, delete it
  if (!SD.remove(path))
    *success = false;

  dbginf("deleting file %s", path);

  targ.close();
}

/*
============================================================================
                                  GET /fs                                   
============================================================================
*/

static void web_server_route_fs_list_files(AsyncWebServerRequest *request)
{
  File target;
  bool is_directory = false;
  const char *path = NULL;
  if (!web_server_parse_file_request(request, &target, &path, &is_directory))
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
      scptr htable_t *f_jsn = web_server_jsonify_file(&curr_file);
      if (jsonh_insert_arr_obj(items, (htable_t *) mman_ref(f_jsn)) != JOPRES_SUCCESS)
        mman_dealloc(f_jsn);
    }

    web_server_json_resp(request, 200, res);
    target.close();
    return;
  }

  // Requested a file, send file contents
  request->send(200, "text/html", target.readString());
  target.close();
}

/*
============================================================================
                                  POST /fs                                  
============================================================================
*/

static void web_server_route_fs_create_file(AsyncWebServerRequest *request)
{
}

/*
============================================================================
                                 DELETE /fs                                 
============================================================================
*/

static void web_server_rec_fdel_req_cleanup(mman_meta_t *meta)
{
  rec_fdel_req *req = (rec_fdel_req *) meta->ptr;
  mman_dealloc(req->path);
}

static void web_server_recursive_delete_task(void *arg)
{
  rec_fdel_req_t *req = (rec_fdel_req_t *) arg;

  bool success = true;
  web_server_recursive_delete(req->path, &success);

  // TODO: If the recursive deletion takes too long, the response socket collapses...
  if (!success)
    web_server_error_resp(req->request, 500, COULD_NOT_DELETE_DIR, "An error ocurred while deleting " QUOTSTR, req->path);
  else
    web_server_empty_ok(req->request);

  mman_dealloc(req);
  vTaskDelete(NULL);
}

static void web_server_route_fs_delete_file(AsyncWebServerRequest *request)
{
  File target;
  bool is_directory = false;
  const char *path = NULL;
  if (!web_server_parse_file_request(request, &target, &path, &is_directory))
    return;

  // File is not needed anymore
  target.close();

  // Requested a directory, list files
  if (is_directory)
  {
    // Create a new file deletion request wrapper struct instance, to be passed as an argument to the task
    scptr rec_fdel_req_t *req = (rec_fdel_req_t *) mman_alloc(sizeof(rec_fdel_req_t), 1, web_server_rec_fdel_req_cleanup);
    req->request = request;
    req->path = strclone(path);

    // Invoke the task on core 1 (since 0 handles the kernel-protocol)
    xTaskCreatePinnedToCore(
      web_server_recursive_delete_task,   // Task entry point
      "rec_fdel",                         // Task name
      16384,                              // Stack size (should be sufficient, I hope)
      mman_ref(req),                      // Parameter to the entry point
      configMAX_PRIORITIES - 1,           // Priority, as high as possible
      NULL,                               // Task handle output, don't care
      1                                   // On core 1 (main loop)
    );
    return;
  }

  // Requested a file, delete the file
  if (!SD.remove(path))
  {
    web_server_error_resp(request, 500, COULD_NOT_DELETE_FILE, "An error ocurred while deleting " QUOTSTR, path);
    return;
  }

  web_server_empty_ok(request);
}

/*
============================================================================
                              Initialization                                
============================================================================
*/

void web_server_route_fs_init(AsyncWebServer *wsrv)
{
  // /fs
  const char *p_sched = "^\\/api\\/fs$";
  wsrv->on(p_sched, HTTP_GET, web_server_route_fs_list_files);
  wsrv->on(p_sched, HTTP_POST, web_server_route_fs_create_file);
  wsrv->on(p_sched, HTTP_DELETE, web_server_route_fs_delete_file);
  wsrv->on(p_sched, HTTP_OPTIONS, web_server_route_any_options);
}