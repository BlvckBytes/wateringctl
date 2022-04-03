#ifndef web_server_common
#define web_server_common

#include "web_server/web_server_error.h"

#include <blvckstd/compattrs.h>
#include <blvckstd/mman.h>
#include <blvckstd/jsonh.h>
#include <ESPAsyncWebServer.h>
#include <SD.h>

// Root directory on the SD for web-files, has to have a trailing /
#define WEB_SERVER_SD_ROOT "/"

/*
============================================================================
                               Success routines                               
============================================================================
*/

/**
 * @brief Send an empty OK (204) to the client
 * 
 * @param request Client request
 */
void web_server_empty_ok(AsyncWebServerRequest *request);

/**
 * @brief Send a JSON response from a jsonh object to the client
 * 
 * @param request Client request
 * @param status Response's statuscode
 * @param json Body json content
 */
void web_server_json_resp(AsyncWebServerRequest *request, int status, htable_t *json);

/*
============================================================================
                               Error routines                               
============================================================================
*/

/**
 * @brief Send a standardized error JSON response to the client
 * 
 * @param request Client request
 * @param status Response's statuscode
 * @param code Ocurred error-code
 * @param fmt Error message printf format
 */
void web_server_error_resp(AsyncWebServerRequest *request, int status, web_server_error_t code, const char *fmt, ...);

/*
============================================================================
                                Body Handling                               
============================================================================
*/

/**
 * @brief Body handler routine to collect segmented string request bodies
 * 
 * @param request Client request
 * @param data Current segment data
 * @param len Current segment data length
 * @param index Index of first byte of current segment inside whole message
 * @param total Total amount of bytes
 */
void web_server_str_body_handler(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);

/**
 * @brief Ensure that a valid JSON body is present and parse it
 * 
 * @param request Client request
 * @param output Jsonh output
 * 
 * @return true JSON OK and parsed
 * @return false JSON invalid, error response has been sent
 */
bool web_server_ensure_json_body(AsyncWebServerRequest *request, htable_t **output);

#endif