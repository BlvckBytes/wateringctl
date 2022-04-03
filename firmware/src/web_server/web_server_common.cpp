#include "web_server/web_server_common.h"

INLINED static void web_server_append_cors_headers(AsyncWebServerResponse *resp)
{
  // Allow CORS requests
  resp->addHeader("Access-Control-Allow-Origin", "*");
  resp->addHeader("Access-Control-Max-Age", "600");
  resp->addHeader("Access-Control-Allow-Methods", "PUT,POST,GET,DELETE,OPTIONS");
  resp->addHeader("Access-Control-Allow-Headers", "*");
}

/*
============================================================================
                               Success routines                               
============================================================================
*/

void web_server_empty_ok(AsyncWebServerRequest *request)
{
  AsyncWebServerResponse *resp = request->beginResponse(204);
  web_server_append_cors_headers(resp);
  request->send(resp);
}

void web_server_json_resp(AsyncWebServerRequest *request, int status, htable_t *json)
{
  scptr char *stringified = jsonh_stringify(json, 2);

  AsyncWebServerResponse *resp = request->beginResponse(status, "application/json", stringified);
  web_server_append_cors_headers(resp);

  request->send(resp);
}

/*
============================================================================
                               Error routines                               
============================================================================
*/

void web_server_error_resp(AsyncWebServerRequest *request, int status, web_server_error_t code, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);

  scptr char *error_msg = vstrfmt_direct(fmt, ap);

  va_end(ap);

  scptr htable_t *resp = jsonh_make();
  scptr char *error_code_name = strclone(web_server_error_name(code));

  jsonh_set_bool(resp, "error", true);
  jsonh_set_str(resp, "code", error_code_name);
  jsonh_set_str(resp, "message", (char *) mman_ref(error_msg));

  web_server_json_resp(request, status, resp);
}

/*
============================================================================
                                Body Handling                               
============================================================================
*/

void web_server_str_body_handler(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
{
  uint8_t **body = (uint8_t **) request->_tempObject;

  // Create buffer on the first segment of the message
  if(index == 0)
  {
    // Allocate using malloc since the API will automatically call free after the request's lifetime
    request->_tempObject = malloc(sizeof(uint8_t **));
    body = (uint8_t **) request->_tempObject;

    // Not enough space available
    if (!body)
      return;

    // Allocate as much memory as the whole body will require
    *body = (uint8_t *) mman_alloc(sizeof(uint8_t), total, NULL);
    request->_tempObject = body;
  }

  // Not enough space for the whole body
  if (!*body)
    return;

  // Write the segment into the buffer
  memcpy(&(body[index]), data, len);
}

INLINED static bool web_server_ensure_str_body(AsyncWebServerRequest *request, char **output)
{
  // Body segment collector has been called at least once
  if (request->_tempObject)
  {
    uint8_t **body = (uint8_t **) request->_tempObject;

    // Check if we had enough space
    if (!*body)
    {
      web_server_error_resp(request, 500, BODY_TOO_LONG, "Body too long, not enough space!");
      return false;
    }

    *output = (char *) *body;
    return true;
  }

  // No body collected
  web_server_error_resp(request, 400, NO_CONTENT, "No body content provided!");
  return false;
}

bool web_server_ensure_json_body(AsyncWebServerRequest *request, htable_t **output)
{
  // Get the string body
  scptr char *body = NULL;
  if (!web_server_ensure_str_body(request, &body))
    return false;

  // Check that the content-type actually matches
  if (request->contentType() != "application/json")
  {
    web_server_error_resp(request, 400, NOT_JSON, "This endpoint only accepts JSON bodies!");
    return false;
  }

  // Parse json
  scptr char *err = NULL;
  scptr htable_t *body_jsn = jsonh_parse(body, &err);
  if (!body_jsn)
  {
    web_server_error_resp(request, 400, INVALID_JSON, "Could not parse the JSON body: %s", err);
    return false;
  }

  // Write output
  *output = (htable_t *) mman_ref(body_jsn);
  return true;
}