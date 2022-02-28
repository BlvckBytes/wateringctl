#include <stdio.h>
#include "util/jsonh.h"

#define EXIT_TEST_FAILURE(varname, retv)                              \
  {                                                                   \
    const char *rets = jsonh_opres_name(retv);                        \
    printf(varname " didn't match the expected value! (%s)\n", rets); \
    return 1;                                                         \
  }

int proc()
{
  scptr htable_t *jsn = jsonh_make();

  // Integer
  jsonh_set_int(jsn, "my-int", 42);
  int my_int = 0;
  jsonh_opres_t int_ret = jsonh_get_int(jsn, "my-int", &my_int);

  if (my_int != 42)
    EXIT_TEST_FAILURE("my-int", int_ret);

  // Boolean
  jsonh_set_bool(jsn, "my-bool", true);
  bool my_bool = false;
  jsonh_opres_t bool_ret = jsonh_get_bool(jsn, "my-bool", &my_bool);

  if (my_bool != true)
    EXIT_TEST_FAILURE("my-bool", bool_ret);

  // Float
  jsonh_set_float(jsn, "my-float", 42.424242);
  float my_float = 0;
  jsonh_opres_t float_ret = jsonh_get_float(jsn, "my-float", &my_float);

  if (my_float >= 43 || my_float <= 41)
    EXIT_TEST_FAILURE("my-float", float_ret);

  // String
  scptr char *my_str = strfmt_direct("Hello, world!");
  jsonh_set_str(jsn, "my-string", (char *) mman_ref(my_str));
  char *my_string = NULL;
  jsonh_opres_t string_ret = jsonh_get_str(jsn, "my-string", &my_string);

  if (strcmp(my_str, my_string) != 0)
    EXIT_TEST_FAILURE("my-string", string_ret);

  // Null
  jsonh_set_null(jsn, "my-null");
  bool my_null = false;
  jsonh_opres_t null_ret = jsonh_get_is_null(jsn, "my-null", &my_null);

  if (my_null != true)
    EXIT_TEST_FAILURE("my-null", null_ret);

  null_ret = jsonh_get_is_null(jsn, "my-string", &my_null);

  if (my_null != false)
    EXIT_TEST_FAILURE("my-string (null check)", null_ret);

  return 0;
}

int main()
{
  int ret = proc();

  if (ret == 0)
    printf("Tests passed!\n");
  else
    printf("Test(s) failed!\n");

  mman_print_info();
  return ret;
}