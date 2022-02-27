#ifndef common_types_h
#define common_types_h

/**
 * @brief Cleanup function used on removal of data
 */
typedef void (*clfn_t)(void *);

/**
 * @brief Stringify a given input used for logging
 */
typedef char *(*stringifier_t)(void *);

#endif