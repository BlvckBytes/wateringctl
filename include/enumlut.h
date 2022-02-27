#ifndef enumlut_h
#define enumlut_h

// Dependencies for the logic inside of macros
#include <stddef.h>
#include <string.h>

// Generate enum, enum value and string enum entries
#define ENUMLUT_GENERATE_ENUM(ENUM, OFFS) ENUM = OFFS,
#define ENUMLUT_GENERATE_STRING(ENUM, OFFS) [OFFS] = #ENUM,
#define ENUMLUT_GENERATE_VALUE(ENUM, OFFS) OFFS,

/*
  INFO: This is a set of macros that - in combination - will set up a full
  INFO: both-ways LUT for enums, which includes the following:

  * Enum typedef                       enum_name##_t
  * Enum names LUT (static)            enum_name##_names
  * Enum values LUT (static)           enum_name##_values
  * Enum get name by value             enum_name##_name(value)
  * Enum get value by name             enum_name##_value(name, out)
  * Enum get length                    enum_name##_length()
  * Enum get value by index            enum_name##_by_index(index, out)
  * 
  * Basically just call it like this:
  * 
  * header file:          ENUM_TYPEDEF_FULL_IMPL(name, values-macro);
  * source file:          ENUM_LUT_FULL_IMPL(name, values-macro);
  * 
  * Where the values macro looks something like this:
  * 
  * #define _EVALS_<name>(FUN)  \
  *   FUN(<a>, 0x01) \
  *   FUN(<b>, 0x02) \
  * ...
*/

/**
 * @brief Result when getting enum values
 */
typedef enum enumlut_result
{
  ENUMLUT_SUCCESS,        // Success, value written to output buffer
  ENUMLUT_NOT_FOUND       // Enum could not be found
} enumlut_result_t;

/**
 * @brief Enum name getter function declaration
 * 
 * @param namearr Name of the array LUT that corresponds numeric
 * values to enum name strings
 * @param enum_name Name of the enum
 */
#define ENUM_GET_NAME_DECL(namearr, enum_name)                    \
  const char *enum_name##_name(enum_name##_t key)

/**
 * @brief Enum name getter function implementation
 * 
 * @param namearr Name of the array LUT that corresponds numeric
 * values to enum name strings
 * @param enum_name Name of the enum
 */
#define ENUM_GET_NAME_IMPL(namearr, enum_name)                    \
  ENUM_GET_NAME_DECL(namearr, enum_name)                          \
  {                                                               \
    /* Calculate the length of the array */                       \
    size_t length = sizeof(namearr) / sizeof(const char *);       \
    /* Key out of range */                                        \
    if (key < 0 || key >= length) return NULL;                    \
    return namearr[key];                                          \
  }

/**
 * @brief Enum value getter function declaration
 * 
 * @param namearr Name of the array LUT that corresponds numeric
 * values to enum name strings
 * @param enum_name Name of the enum
 */
#define ENUM_GET_VALUE_DECL(namearr, enum_name)                   \
  enumlut_result_t enum_name##_value(const char *name, enum_name##_t *out)

/**
 * @brief Enum value getter function implementation
 * 
 * @param namearr Name of the array LUT that corresponds numeric
 * values to enum name strings
 * @param enum_name Name of the enum
 */
#define ENUM_GET_VALUE_IMPL(namearr, enum_name)                   \
  ENUM_GET_VALUE_DECL(namearr, enum_name)                         \
  {                                                               \
    /* No name provided */                                        \
    if (!name) return ENUMLUT_NOT_FOUND;                          \
    /* Calculate the length of the array */                       \
    size_t length = sizeof(namearr) / sizeof(const char *);       \
    /* Search through all keys */                                 \
    for (size_t i = 0; i < length; i++)                           \
    {                                                             \
      /* Compare and return if the name matched */                \
      const char *curr = namearr[i];                              \
      if (curr && strcasecmp(name, curr) == 0)                    \
      {                                                           \
        *out = (enum_name##_t) i;                                 \
        return ENUMLUT_SUCCESS;                                   \
      }                                                           \
    }                                                             \
    /* Key not found */                                           \
    return ENUMLUT_NOT_FOUND;                                     \
  }

/**
 * @brief Enum value by enum list index getter function declaration
 * 
 * @param namearr Name of the array LUT that corresponds numeric
 * indices to numeric enum values
 * @param enum_name Name of the enum
 */
#define ENUM_GET_BY_INDEX_DECL(namearr, enum_name)                \
  enumlut_result_t enum_name##_by_index(size_t index, enum_name##_t *out)

/**
 * @brief Enum value by enum list index getter function implementation
 * 
 * @param namearr Name of the array LUT that corresponds numeric
 * indices to numeric enum values
 * @param enum_name Name of the enum
 */
#define ENUM_GET_BY_INDEX_IMPL(namearr, enum_name)                \
  ENUM_GET_BY_INDEX_DECL(namearr, enum_name)                      \
  {                                                               \
    if (index >= enum_name##_length()) return ENUMLUT_NOT_FOUND;  \
    *out = (enum_name##_t) enum_name##_values[index];             \
    return ENUMLUT_SUCCESS;                                       \
  }

/**
 * @brief Generate the implementation for an enum lookup table
 * 
 * @param enum_name Name of the enum
 * @param declfunc Makro function list that declares the values
 */
#define ENUM_LUT_IMPL(enum_name, declfunc)                        \
  static const char *enum_name##_names[] = {                      \
    declfunc(ENUMLUT_GENERATE_STRING)                             \
  };

/**
 * @brief Generate the implementation for an enum value by index lookup table
 * 
 * @param enum_name Name of the enum
 * @param declfunc Makro function list that declares the values
 */
#define ENUM_VALS_IMPL(enum_name, declfunc)                       \
  static size_t enum_name##_values[] = {                          \
    declfunc(ENUMLUT_GENERATE_VALUE)                              \
  };

/**
 * @brief Enum length (number of total entries) getter function declaration
 * 
 * @param namearr Name of the array LUT that corresponds numeric
 * indices to numeric enum values
 * @param enum_name Name of the enum
 */
#define ENUM_GET_LEN_DECL(namearr, enum_name)                     \
  size_t enum_name##_length()

/**
 * @brief Enum length (number of total entries) getter function implementation
 * 
 * @param namearr Name of the array LUT that corresponds numeric
 * indices to numeric enum values
 * @param enum_name Name of the enum
 */
#define ENUM_GET_LEN_IMPL(namearr, enum_name)                     \
  ENUM_GET_LEN_DECL(namearr, enum_name)                           \
  {                                                               \
    return sizeof(enum_name##_values) / sizeof(size_t);           \
  }

/**
 * @brief Generate the enum typedefinition
 * 
 * @param enum_name Name of the enum
 * @param declfunc Makro function list that declares the values
 */
#define ENUM_TYPDEF_IMPL(enum_name, declfunc)                     \
  typedef enum enum_name                                          \
  {                                                               \
    declfunc(ENUMLUT_GENERATE_ENUM)                               \
  } enum_name##_t;

/**
 * @brief Generate the full LUT implementation, including LUT as
 * well as the name and value getter functions
 * 
 * INFO: Call this in the C file
 * 
 * @param enum_type Typedef symbol of the enum
 * @param declfunc Makro function list that declares the values
 */
#define ENUM_LUT_FULL_IMPL(enum_name, declfunc)                   \
  ENUM_LUT_IMPL(enum_name, declfunc);                             \
  ENUM_VALS_IMPL(enum_name, declfunc);                            \
  ENUM_GET_NAME_IMPL(enum_name##_names, enum_name);               \
  ENUM_GET_VALUE_IMPL(enum_name##_names, enum_name);              \
  ENUM_GET_LEN_IMPL(enum_name##_values, enum_name);               \
  ENUM_GET_BY_INDEX_IMPL(enum_name##_values, enum_name);

/**
 * @brief Generate the full typedef implementation, including typedefinition
 * as well as the name and value getter function declarations
 * 
 * INFO: Call this in the header file
 * 
 * @param enum_type Typedef symbol of the enum
 * @param declfunc Makro function list that declares the values
 */
#define ENUM_TYPEDEF_FULL_IMPL(enum_name, declfunc)               \
  ENUM_TYPDEF_IMPL(enum_name, declfunc);                          \
  ENUM_GET_NAME_DECL(enum_name##_names, enum_name);               \
  ENUM_GET_VALUE_DECL(enum_name##_names, enum_name);              \
  ENUM_GET_BY_INDEX_DECL(enum_name##_values, enum_name);          \
  ENUM_GET_LEN_DECL(enum_name##_values, enum_name);

#endif