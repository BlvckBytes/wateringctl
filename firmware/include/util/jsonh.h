#ifndef jsonh_h
#define jsonh_h

/*
  Json parser, stringifier and just general handler.

  Parsing tries to orient itself by https://www.json.org/json-en.html
*/

#include <stdarg.h>

#include "util/htable.h"
#include "util/strfmt.h"
#include "util/dynarr.h"
#include "util/enumlut.h"

// All possible datatypes of a JSON value
#define _EVALS_JSONH_DTYPE(FUN)   \
  FUN(JDTYPE_STR,     0x0)        \
  FUN(JDTYPE_INT,     0x1)        \
  FUN(JDTYPE_FLOAT,   0x2)        \
  FUN(JDTYPE_OBJ,     0x3)        \
  FUN(JDTYPE_ARR,     0x4)        \
  FUN(JDTYPE_BOOL,    0x5)        \
  FUN(JDTYPE_NULL,    0x6)

#define _EVALS_JSONH_LITERAL(FUN) \
  FUN(JLIT_TRUE,      0x0)        \
  FUN(JLIT_FALSE,     0x1)        \
  FUN(JLIT_NULL,      0x2)

#define _EVALS_JSONH_OPRES(FUN)                                                               \
  FUN(JOPRES_SUCCESS,         0x0) /* Successful operation */                                 \
  FUN(JOPRES_DTYPE_MISMATCH,  0x1) /* Datatype of value didn't match datatype of getter */    \
  FUN(JOPRES_SIZELIM_EXCEED,  0x2) /* An internal error occurred */                           \
  FUN(JOPRES_INVALID_INDEX,   0x3) /* The requested array-index is out of range */            \
  FUN(JOPRES_INVALID_KEY,     0x4) /* The requested key does not exist */

#define JSONH_ROOT_ITEM_CAP 1024

ENUM_TYPEDEF_FULL_IMPL(jsonh_datatype, _EVALS_JSONH_DTYPE);
ENUM_TYPEDEF_FULL_IMPL(jsonh_literal, _EVALS_JSONH_LITERAL);
ENUM_TYPEDEF_FULL_IMPL(jsonh_opres, _EVALS_JSONH_OPRES);

/**
 * @brief This wraps all values (htable and dynarr) to allow
 * for datatype detection at runtime
 */
typedef struct jsonh_value
{
  jsonh_datatype_t type;
  void *value;
} jsonh_value_t;

jsonh_value_t *jsonh_value_make(void *val, jsonh_datatype_t val_type);

/*
============================================================================
                                 Creation                                   
============================================================================
*/

/**
 * @brief Create a new json handler
 */
htable_t *jsonh_make();

/*
============================================================================
                                  Parsing                                   
============================================================================
*/

typedef struct jsonh_cursor
{
  const char *text;         // Text which is managed by the cursor
  long text_length;        // Number of characters in the text
  long text_index;         // Current character index within the text

  long char_index;         // Index of the current character in the current line
  long line_index;         // Index of the current line in the full text
} jsonh_cursor_t;

typedef struct jsonh_char
{
  char c;         // The char itself
  bool is_esc;    // Whether or not it's escaped
} jsonh_char_t;

/**
 * @brief Create a new cursor for a given pice of text
 * 
 * @param text Text to curse over
 * @return jsonh_cursor_t* New cursor with all defaults initialized
 */
jsonh_cursor_t *jsonh_cursor_make(const char *text);

/**
 * @brief Get the next character from the cursor
 * 
 * @param cursor Cursor handle
 * @return char Next character, 0 means EOF
 */
jsonh_char_t jsonh_cursor_getc(jsonh_cursor_t *cursor);

/**
 * @brief Get the next character from the cursor
 * without changing it's state
 * 
 * @param cursor Cursor handle
 * @return char Next character, 0 means EOF
 */
jsonh_char_t jsonh_cursor_peekc(jsonh_cursor_t *cursor);

/**
 * @brief Undo the last call of getc, does nothing if it's already
 * at the beginning
 * 
 * @param cursor Cursor handle
 */
void jsonh_cursor_ungetc(jsonh_cursor_t *cursor);

/**
 * @brief Parse a JSON array: [ 0, 1, 2, ... ]
 * 
 * @param cursor Cursor handle
 * @param err Error output buffer
 * @param out Output value buffer
 * 
 * @return true Parsing succeeded
 * @return false Parsing failed
 */
bool jsonh_parse_arr(jsonh_cursor_t *cursor, char **err, dynarr_t **out);

/**
 * @brief Parse a JSON object: { "a": "b", ... }
 * 
 * @param cursor Cursor handle
 * @param err Error output buffer
 * @param out Output value buffer
 * 
 * @return true Parsing succeeded
 * @return false Parsing failed
 */
bool jsonh_parse_obj(jsonh_cursor_t *cursor, char **err, htable_t **out);

/**
 * @brief Parse a JSON value: string, number, object, array, literal
 * 
 * @param cursor Cursor handle
 * @param err Error output buffer
 * @param out Output value buffer
 * 
 * @return true Parsing succeeded
 * @return false Parsing failed
 */
bool jsonh_parse_value(jsonh_cursor_t *cursor, char **err, jsonh_value_t *out);

/**
 * @brief Parse a JSON literal: true, false, null
 * 
 * @param cursor Cursor handle
 * @param err Error output buffer
 * @param out Output value buffer
 * 
 * @return true Parsing succeeded
 * @return false Parsing failed
 */
bool jsonh_parse_literal(jsonh_cursor_t *cursor, char **err, jsonh_literal_t *out);

/**
 * @brief Parse a JSON number: 5 or 5.5
 * 
 * @param cursor Cursor handle
 * @param err Error output buffer
 * @param out Output value buffer
 * 
 * @return true Parsing succeeded
 * @return false Parsing failed
 */
bool jsonh_parse_num(jsonh_cursor_t *cursor, char **err, double *out, bool *had_dot);

/**
 * @brief Parse a JSON string: "..."
 * 
 * @param cursor Cursor handle
 * @param err Error output buffer
 * @param out Output value buffer
 * 
 * @return true Parsing succeeded
 * @return false Parsing failed
 */
bool jsonh_parse_str(jsonh_cursor_t *cursor, char **err, char **out);

/**
 * @brief Eat up all whitespace (non-printable and spaces) by
 * calling getc until a printable character appears
 * 
 * @param cursor Cursor handle
 */
void jsonh_parse_eat_whitespace(jsonh_cursor_t *cursor);

/**
 * @brief Parse a JSON string into a datastructure
 * 
 * @param json JSON string to parse
 * @param err Error output
 * 
 * @return htable_t* Parsed JSON or NULL on errors (see err)
 */
htable_t *jsonh_parse(const char *json, char **err);

/*
============================================================================
                                 Stringify                                  
============================================================================
*/

/**
 * @brief Stringify a json handler's current value into a JSON string
 * 
 * @param jsonh Json handler instance
 * @param indent Number of spaces a level of indentation is made of
 * 
 * @return char* Stringified result
 */
char *jsonh_stringify(htable_t *jsonh, int indent);

/*
============================================================================
                                 Setters                                    
============================================================================
*/

/**
 * @brief Set the value of a given key to another JSON object
 * 
 * @param jsonh Json handler instance
 * @param key Key to set
 * @param obj JSON object
 * 
 * @return jsonh_opres_t Operation result
 */
jsonh_opres_t jsonh_set_obj(htable_t *jsonh, const char *key, htable_t *obj);

/**
 * @brief Set the value of a given key to a string
 * 
 * @param jsonh Json handler instance
 * @param key Key to set
 * @param obj String value
 * 
 * @return jsonh_opres_t Operation result
 */
jsonh_opres_t jsonh_set_str(htable_t *jsonh, const char *key, char *str);

/**
 * @brief Set the value of a given key to an integer
 * 
 * @param jsonh Json handler instance
 * @param key Key to set
 * @param obj Integer value
 * 
 * @return jsonh_opres_t Operation result
 */
jsonh_opres_t jsonh_set_int(htable_t *jsonh, const char *key, int num);

/**
 * @brief Set the value of a given key to a float
 * 
 * @param jsonh Json handler instance
 * @param key Key to set
 * @param obj Float value
 * 
 * @return jsonh_opres_t Operation result
 */
jsonh_opres_t jsonh_set_float(htable_t *jsonh, const char *key, float num);

/**
 * @brief Set the value of a given key to a bool
 * 
 * @param jsonh Json handler instance
 * @param key Key to set
 * @param obj Boolean value
 * 
 * @return jsonh_opres_t Operation result
 */
jsonh_opres_t jsonh_set_bool(htable_t *jsonh, const char *key, bool b);

/**
 * @brief Set the value of a given key to null
 * 
 * @param jsonh Json handler instance
 * @param key Key to set
 * 
 * @return jsonh_opres_t Operation result
 */
jsonh_opres_t jsonh_set_null(htable_t *jsonh, const char *key);

/**
 * @brief Set the value of a given key to a JSON array
 * 
 * @param jsonh Json handler instance
 * @param key Key to set
 * @param obj JSON array
 * 
 * @return jsonh_opres_t Operation result
 */
jsonh_opres_t jsonh_set_arr(htable_t *jsonh, const char *key, dynarr_t *arr);

/*
============================================================================
                              Array Setters                                 
============================================================================
*/

/**
 * @brief Insert an object into a JSON array
 * 
 * @param array Json array instance
 * @param obj Object to insert
 * 
 * @return jsonh_opres_t Operation result
 */
jsonh_opres_t jsonh_insert_arr_obj(dynarr_t *array, htable_t *obj);

/**
 * @brief Insert an array into a JSON array
 * 
 * @param array Json array instance
 * @param arr Array to insert
 * 
 * @return jsonh_opres_t Operation result
 */
jsonh_opres_t jsonh_insert_arr_arr(dynarr_t *array, dynarr_t *arr);

/**
 * @brief Insert a string into a JSON array
 * 
 * @param array Json array instance
 * @param str String to insert
 * 
 * @return jsonh_opres_t Operation result
 */
jsonh_opres_t jsonh_insert_arr_str(dynarr_t *array, char *str);

/**
 * @brief Insert an int into a JSON array
 * 
 * @param array Json array instance
 * @param num Int to insert
 * 
 * @return jsonh_opres_t Operation result
 */
jsonh_opres_t jsonh_insert_arr_int(dynarr_t *array, int num);

/**
 * @brief Insert a float into a JSON array
 * 
 * @param array Json array instance
 * @param num Float to insert
 * 
 * @return jsonh_opres_t Operation result
 */
jsonh_opres_t jsonh_insert_arr_float(dynarr_t *array, float num);

/**
 * @brief Insert a boolean into a JSON array
 * 
 * @param array Json array instance
 * @param b Boolean to insert
 * 
 * @return jsonh_opres_t Operation result
 */
jsonh_opres_t jsonh_insert_arr_bool(dynarr_t *array, bool b);

/**
 * @brief Insert null into a JSON array
 * 
 * @param array Json array instance
 * 
 * @return jsonh_opres_t Operation result
 */
jsonh_opres_t jsonh_insert_arr_null(dynarr_t *array);

/*
============================================================================
                                 Getters                                    
============================================================================
*/

/**
 * @brief Get the value of a given key as another JSON object
 * 
 * @param jsonh Json handler instance
 * @param key Key to get
 * @param obj JSON object output
 * 
 * @return jsonh_opres_t Operation result
 */
jsonh_opres_t jsonh_get_obj(htable_t *jsonh, const char *key, htable_t **obj);

/**
 * @brief Get the value of a given key as a JSON array
 * 
 * @param jsonh Json handler instance
 * @param key Key to get
 * @param obj JSON array output
 * 
 * @return jsonh_opres_t Operation result
 */
jsonh_opres_t jsonh_get_arr(htable_t *jsonh, const char *key, dynarr_t **arr);

/**
 * @brief Get the value of a given key as a string
 * 
 * @param jsonh Json handler instance
 * @param key Key to get
 * @param obj String value output
 * 
 * @return jsonh_opres_t Operation result
 */
jsonh_opres_t jsonh_get_str(htable_t *jsonh, const char *key, char **str);

/**
 * @brief Get the value of a given key as an integer
 * 
 * @param key Key to get
 * @param obj Integer value output
 * 
 * @return jsonh_opres_t Operation result
 */
jsonh_opres_t jsonh_get_int(htable_t *jsonh, const char *key, int *num);

/**
 * @brief Get the value of a given key as a float
 * 
 * @param jsonh Json handler instance
 * @param key Key to get
 * @param obj Float value output
 * 
 * @return jsonh_opres_t Operation result
 */
jsonh_opres_t jsonh_get_float(htable_t *jsonh, const char *key, float *num);

/**
 * @brief Get the value of a given key as a bool
 * 
 * @param key Key to get
 * @param obj Boolean value output
 * 
 * @return jsonh_opres_t Operation result
 */
jsonh_opres_t jsonh_get_bool(htable_t *jsonh, const char *key, bool *b);

/**
 * @brief Get if the value of a given key is null
 * 
 * @param jsonh Json handler instance
 * @param key Key to get
 * @param is_null Boolean state output
 * 
 * @return jsonh_opres_t Operation result
 */
jsonh_opres_t jsonh_get_is_null(htable_t *jsonh, const char *key, bool *is_null);

/*
============================================================================
                              Array Getters                                 
============================================================================
*/

/**
 * @brief Get the value of a slot as an object
 * 
 * @param array Array instance
 * @param index Index of the target slot
 * @param obj Object output
 * 
 * @return jsonh_opres_t Operation result
 */
jsonh_opres_t jsonh_get_arr_obj(dynarr_t *array, int index, htable_t *obj);

/**
 * @brief Get the value of a slot as an array
 * 
 * @param array Array instance
 * @param index Index of the target slot
 * @param arr Array output
 * 
 * @return jsonh_opres_t Operation result
 */
jsonh_opres_t jsonh_get_arr_arr(dynarr_t *array, int index, dynarr_t *arr);

/**
 * @brief Get the value of a slot as a string
 * 
 * @param array Array instance
 * @param index Index of the target slot
 * @param str String output
 * 
 * @return jsonh_opres_t Operation result
 */
jsonh_opres_t jsonh_get_arr_str(dynarr_t *array, int index, char *str);

/**
 * @brief Get the value of a slot as an integer
 * 
 * @param array Array instance
 * @param index Index of the target slot
 * @param num Integer output
 * 
 * @return jsonh_opres_t Operation result
 */
jsonh_opres_t jsonh_get_arr_int(dynarr_t *array, int index, int *num);

/**
 * @brief Get the value of a slot as a float
 * 
 * @param array Array instance
 * @param index Index of the target slot
 * @param num Float output
 * 
 * @return jsonh_opres_t Operation result
 */
jsonh_opres_t jsonh_get_arr_float(dynarr_t *array, int index, float *num);

/**
 * @brief Get the value of a slot as a boolean
 * 
 * @param array Array instance
 * @param index Index of the target slot
 * @param b Boolean output
 * 
 * @return jsonh_opres_t Operation result
 */
jsonh_opres_t jsonh_get_arr_bool(dynarr_t *array, int index, bool *b);

/**
 * @brief Get if the value of a slot is null
 * 
 * @param array Array instance
 * @param index Index of the target slot
 * @param is_null Boolean state output
 * 
 * @return jsonh_opres_t Operation result
 */
jsonh_opres_t jsonh_get_arr_is_null(dynarr_t *array, int index, bool *is_null);

#endif