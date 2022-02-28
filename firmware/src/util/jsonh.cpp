#include "util/jsonh.h"

ENUM_LUT_FULL_IMPL(jsonh_datatype, _EVALS_JSONH_DTYPE);
ENUM_LUT_FULL_IMPL(jsonh_opres, _EVALS_JSONH_OPRES);

/*
============================================================================
                                 Creation                                   
============================================================================
*/

htable_t *jsonh_make()
{
  scptr htable_t *res = htable_make(JSONH_ROOT_ITEM_CAP, mman_dealloc_nr);
  return (htable_t *) mman_ref(res);
}

/*
============================================================================
                                  Parsing                                   
============================================================================
*/

// TODO: Implement parsing

/*
============================================================================
                                 Stringify                                  
============================================================================
*/

// Hoisted declarations
static void jsonh_stringify_obj(htable_t *obj, int indent, int indent_level, char **buf, size_t *buf_offs);
static void jsonh_stringify_arr(dynarr_t *arr, int indent, int indent_level, char **buf, size_t *buf_offs);

static char *jsonh_gen_indent(int indent)
{
  scptr char *buf = (char *) mman_alloc(sizeof(char), indent + 1, NULL);
  for (int i = 0; i < indent; i++)
    buf[i] = ' ';
  buf[indent] = 0;
  return (char *) mman_ref(buf);
}

/**
 * @brief Stringify a json-value based on it's type marker field
 * 
 * @param jv Jason value
 * @param indent Size of an indentation
 * @param indent_level Current level of indentation
 * @param buf String buffer to write into
 * @param buf_offs Current buffer offset, will be altered
 */
static void jsonh_stringify_value(jsonh_value_t *jv, int indent, int indent_level, char **buf, size_t *buf_offs)
{
  // Decide on type and call matching stfingifier
  switch (jv->type)
  {
    case JDTYPE_BOOL:
    strfmt(buf, buf_offs, "%s", *((bool *) jv->value) ? "true" : "false");
    return;

    case JDTYPE_INT:
    strfmt(buf, buf_offs, "%d", *((int *) jv->value));
    return;

    case JDTYPE_FLOAT:
    strfmt(buf, buf_offs, "%.7f", *((float *) jv->value));
    return;

    case JDTYPE_ARR:
    jsonh_stringify_arr((dynarr_t *) jv->value, indent, indent_level + 1, buf, buf_offs);
    return;

    case JDTYPE_OBJ:
    jsonh_stringify_obj((htable_t *) jv->value, indent, indent_level + 1, buf, buf_offs);
    return;

    case JDTYPE_NULL:
    strfmt(buf, buf_offs, "null");
    return;

    case JDTYPE_STR:
    strfmt(buf, buf_offs, QUOTSTR, (char *) jv->value);
    return;
  }
}

/**
 * @brief Stringify an array to it's corresponding JSON value
 * 
 * Example value:
 * {
 *   "a": "b",
 *   "c": "d",
 *   "e": 55
 * }
 * 
 * @param arr Array to stringify
 * @param indent Size of an indentation
 * @param indent_level Current level of indentation
 * @param buf String buffer to write into
 * @param buf_offs Current buffer offset, will be altered
 */
static void jsonh_stringify_obj(htable_t *obj, int indent, int indent_level, char **buf, size_t *buf_offs)
{
  strfmt(buf, buf_offs, "{\n");
  scptr char *indent_str = jsonh_gen_indent(indent * indent_level);
  scptr char *indent_str_outer = jsonh_gen_indent(indent * u64_max(0, (uint64_t) indent_level - 1U));

  // Get all object keys
  scptr char **keys = NULL;
  htable_list_keys(obj, &keys);

  // Iterate object keys
  for (char **key = keys; *key; key++)
  {
    jsonh_value_t *jv = NULL;
    if (htable_fetch(obj, *key, (void **) &jv) != HTABLE_SUCCESS)
      continue;

    const char *sep = *(key + 1) == NULL ? "" : ",";
    strfmt(buf, buf_offs, "%s" QUOTSTR ": ", indent_str, *key);
    jsonh_stringify_value(jv, indent, indent_level, buf, buf_offs);
    strfmt(buf, buf_offs, "%s\n", sep);
  }

  strfmt(buf, buf_offs, "%s}", indent_str_outer);
}

/**
 * @brief Stringify an array to it's corresponding JSON value
 * 
 * Example value:
 * [
 *   1,
 *   2,
 *   3
 * ]
 * 
 * @param arr Array to stringify
 * @param indent Size of an indentation
 * @param indent_level Current level of indentation
 * @param buf String buffer to write into
 * @param buf_offs Current buffer offset, will be altered
 */
static void jsonh_stringify_arr(dynarr_t *arr, int indent, int indent_level, char **buf, size_t *buf_offs)
{
  strfmt(buf, buf_offs, "[\n");
  scptr char *indent_str = jsonh_gen_indent(indent * indent_level);
  scptr char *indent_str_outer = jsonh_gen_indent(indent * u64_max(0, (uint64_t) indent_level - 1U));

  // Get all array values
  scptr void **arr_vals = NULL;
  dynarr_as_array(arr, &arr_vals);
  
  // Iterate array values
  for (void **arr_v = arr_vals; *arr_v; arr_v++)
  {
    jsonh_value_t *jv = (jsonh_value_t *) *arr_v;
    const char *sep = *(arr_v + 1) == NULL ? "" : ",";
    strfmt(buf, buf_offs, "%s", indent_str);
    jsonh_stringify_value(jv, indent, indent_level, buf, buf_offs);
    strfmt(buf, buf_offs, "%s\n", sep);
  }

  strfmt(buf, buf_offs, "%s]", indent_str_outer);
}

char *jsonh_stringify(htable_t *jsonh, int indent)
{
  scptr char *buf = (char *) mman_alloc(sizeof(char), 512, NULL);
  size_t buf_offs = 0;

  jsonh_stringify_obj(jsonh, indent, 1, &buf, &buf_offs);
  strfmt(&buf, &buf_offs, "\n");

  return (char *) mman_ref(buf);
}

/*
============================================================================
                                 Setters                                    
============================================================================
*/

static void jsonh_value_cleanup(mman_meta_t *meta)
{
  jsonh_value_t *value = (jsonh_value_t *) meta->ptr;
  mman_dealloc(value->value);
}

static jsonh_value_t *jsonh_value_make(void *val, jsonh_datatype_t val_type)
{
  scptr jsonh_value_t *value = (jsonh_value_t *) mman_alloc(sizeof(jsonh_value_t), 1, jsonh_value_cleanup);
  value->type = val_type;
  value->value = val;
  return (jsonh_value_t *) mman_ref(value);
}

static jsonh_opres_t jsonh_set_value(htable_t *jsonh, const char *key, void *val, jsonh_datatype_t val_type)
{
  scptr jsonh_value_t* value = jsonh_value_make(val, val_type);
  if (htable_insert(jsonh, key, mman_ref(value)) == HTABLE_SUCCESS)
    return JOPRES_SUCC;

  mman_dealloc(value);
  return JOPRES_INTERNAL_ERR;
}

static jsonh_opres_t jsonh_insert_value(dynarr_t *array, void *val, jsonh_datatype_t val_type)
{
  scptr jsonh_value_t* value = jsonh_value_make(val, val_type);
  if (dynarr_push(array, mman_ref(value), NULL) == DYNARR_SUCCESS)
    return JOPRES_SUCC;

  mman_dealloc(value);
  return JOPRES_INTERNAL_ERR;
}

jsonh_opres_t jsonh_set_obj(htable_t *jsonh, const char *key, htable_t *obj)
{
  return jsonh_set_value(jsonh, key, (void *) obj, JDTYPE_OBJ);
}

jsonh_opres_t jsonh_set_str(htable_t *jsonh, const char *key, char *str)
{
  return jsonh_set_value(jsonh, key, (void *) str, JDTYPE_STR);
}

jsonh_opres_t jsonh_set_int(htable_t *jsonh, const char *key, int num)
{
  scptr int *numv = (int *) mman_alloc(sizeof(int), 1, NULL);
  *numv = num;

  jsonh_opres_t ret = jsonh_set_value(jsonh, key, mman_ref(numv), JDTYPE_INT);
  if (ret != JOPRES_SUCC)
    mman_dealloc(numv);

  return ret;
}

jsonh_opres_t jsonh_set_float(htable_t *jsonh, const char *key, float num)
{
  scptr float *numv = (float *) mman_alloc(sizeof(float), 1, NULL);
  *numv = num;

  jsonh_opres_t ret = jsonh_set_value(jsonh, key, mman_ref(numv), JDTYPE_FLOAT);
  if (ret != JOPRES_SUCC)
    mman_dealloc(numv);

  return ret;
}

jsonh_opres_t jsonh_set_bool(htable_t *jsonh, const char *key, bool b)
{
  scptr bool *boolv = (bool *) mman_alloc(sizeof(bool), 1, NULL);
  *boolv = b;

  jsonh_opres_t ret = jsonh_set_value(jsonh, key, mman_ref(boolv), JDTYPE_BOOL);
  if (ret != JOPRES_SUCC)
    mman_dealloc(boolv);

  return ret;
}

jsonh_opres_t jsonh_set_null(htable_t *jsonh, const char *key)
{
  return jsonh_set_value(jsonh, key, NULL, JDTYPE_NULL);
}

jsonh_opres_t jsonh_set_arr(htable_t *jsonh, const char *key, dynarr_t *arr)
{
  return jsonh_set_value(jsonh, key, (void *) arr, JDTYPE_ARR);
}

/*
============================================================================
                             Array Insertion                                
============================================================================
*/

jsonh_opres_t jsonh_insert_arr_obj(dynarr_t *array, htable_t *obj)
{
  return jsonh_insert_value(array, (void *) obj, JDTYPE_OBJ);
}

jsonh_opres_t jsonh_insert_arr_arr(dynarr_t *array, dynarr_t *arr)
{
  return jsonh_insert_value(array, (void *) arr, JDTYPE_ARR);
}

jsonh_opres_t jsonh_insert_arr_str(dynarr_t *array, char *str)
{
  return jsonh_insert_value(array, (void *) str, JDTYPE_STR);
}

jsonh_opres_t jsonh_insert_arr_int(dynarr_t *array, int num)
{
  scptr int *numv = (int *) mman_alloc(sizeof(int), 1, NULL);
  *numv = num;

  jsonh_opres_t ret = jsonh_insert_value(array, mman_ref(numv), JDTYPE_INT);
  if (ret != JOPRES_SUCC)
    mman_dealloc(numv);

  return ret;
}

jsonh_opres_t jsonh_insert_arr_float(dynarr_t *array, float num)
{
  scptr float *numv = (float *) mman_alloc(sizeof(float), 1, NULL);
  *numv = num;

  jsonh_opres_t ret = jsonh_insert_value(array, mman_ref(numv), JDTYPE_FLOAT);
  if (ret != JOPRES_SUCC)
    mman_dealloc(numv);

  return ret;
}

jsonh_opres_t jsonh_insert_arr_bool(dynarr_t *array, bool b)
{
  scptr bool *boolv = (bool *) mman_alloc(sizeof(bool), 1, NULL);
  *boolv = b;

  jsonh_opres_t ret = jsonh_insert_value(array, mman_ref(boolv), JDTYPE_BOOL);
  if (ret != JOPRES_SUCC)
    mman_dealloc(boolv);

  return ret;
}

jsonh_opres_t jsonh_insert_arr_null(dynarr_t *array)
{
  return jsonh_insert_value(array, NULL, JDTYPE_NULL);
}
