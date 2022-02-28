#include <stdio.h>
#include "util/jsonh.h"

int proc()
{
  scptr htable_t *jsn = jsonh_make();

  // Bool
  jsonh_set_bool(jsn, "my-bool", true);
  jsonh_set_bool(jsn, "your-bool", false);

  // Null
  jsonh_set_null(jsn, "null-val");

  // Float
  jsonh_set_float(jsn, "float-val", 1.234567890);

  // Integer
  jsonh_set_int(jsn, "int-val", 128);

  // String
  scptr char *str = strfmt_direct("%s", "Hello, world! This is a string");
  jsonh_set_str(jsn, "string-val", (char *) mman_ref(str));

  // Array of all same type - string
  scptr dynarr_t *arr = dynarr_make(8, 32, mman_dealloc_nr);
  jsonh_insert_arr_str(arr, strfmt_direct("Array item 1"));
  jsonh_insert_arr_str(arr, strfmt_direct("Array item 2"));
  jsonh_insert_arr_str(arr, strfmt_direct("Array item 3"));
  jsonh_set_arr(jsn, "nice-arr", (dynarr_t *) mman_ref(arr));

  // Object
  scptr htable_t *jsn_inner = jsonh_make();
  jsonh_set_int(jsn_inner, "inner-int", 55);
  jsonh_set_float(jsn_inner, "inner-float", 55.232);
  jsonh_set_bool(jsn_inner, "inner-bool", true);
  jsonh_set_obj(jsn, "inner-object", (htable_t *) mman_ref(jsn_inner));

  // Array of different types
  // Appended on inner object
  scptr dynarr_t *arr_w = dynarr_make(8, 32, mman_dealloc_nr);
  jsonh_insert_arr_str(arr_w, strfmt_direct("Array item 1"));
  jsonh_insert_arr_int(arr_w, 55);
  jsonh_insert_arr_float(arr_w, 52.23333333);
  jsonh_insert_arr_bool(arr_w, false);
  jsonh_set_arr(jsn_inner, "weird-arr", (dynarr_t *) mman_ref(arr_w));

  // Stringify
  scptr char *stringified = jsonh_stringify(jsn, 2);
  printf("%s", stringified);

  const char *expected = \
  "{\n"
  "  \"my-bool\": true,\n"
  "  \"int-val\": 128,\n"
  "  \"nice-arr\": [\n"
  "    \"Array item 1\",\n"
  "    \"Array item 2\",\n"
  "    \"Array item 3\"\n"
  "  ],\n"
  "  \"inner-object\": {\n"
  "    \"weird-arr\": [\n"
  "      \"Array item 1\",\n"
  "      55,\n"
  "      52.2333336,\n"
  "      false\n"
  "    ],\n"
  "    \"inner-bool\": true,\n"
  "    \"inner-int\": 55,\n"
  "    \"inner-float\": 55.2319984\n"
  "  },\n"
  "  \"float-val\": 1.2345679,\n"
  "  \"string-val\": \"Hello, world! This is a string\",\n"
  "  \"null-val\": null,\n"
  "  \"your-bool\": false\n"
  "}\n";

  printf("Test passed: %s\n", strcmp(expected, stringified) == 0 ? "YES" : "NO");
  return 0;
}

int main()
{
  int ret = proc();
  mman_print_info();
  return ret;
}