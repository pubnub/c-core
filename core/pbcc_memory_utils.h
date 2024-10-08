#ifndef PBCC_MEMORY_UTILS_H
#define PBCC_MEMORY_UTILS_H


/**
 * @file  pbcc_memory_utils.h
 * @brief Module with utility function and macros to work with memory.
 */

#include <stdbool.h>
#include "core/pubnub_log.h"


// ----------------------------------------------
//                     Macros
// ----------------------------------------------


/**
 * @brief Macro for type allocation and error handling.
 * @code
 * my_user_t* db_user_allocate(db_t* db, char* name) {
 *     PBCC_ALLOCATE_TYPE(user, my_user_t, false, NULL);
 *     user->f_name = strdup(name);
 *     db_connect_store(db, user);
 *
 *     return user;
 * }
 * @endcode
 *
 * @param var                 The name of the variable which will be assigned to
 *                            the allocated memory pointer.
 * @param type                Name of type for which memory will be allocated.
 * @param print_error_message Whether a memory allocation error should be
 *                            printed to the console and log file or not.
 * @param return_value        What should be returned from the function which
 *                            use macro in case of a memory allocation error.
 *                            The value can be 'NULL' or 'enum' field .
 */
#define PBCC_ALLOCATE_TYPE(var, type, print_error_message, return_value) \
    type *var = (type *)malloc(sizeof(type));                            \
    if (NULL == var) {                                                   \
        if (true == print_error_message) {                               \
            PUBNUB_LOG_ERROR("[%s] Failed to allocate memory for '%s'\n",\
                             __func__, #type);                           \
        }                                                                \
        return return_value;                                             \
    }


// ----------------------------------------------
//                   Functions
// ----------------------------------------------

/**
 * @brief Clean up resources allocated for pointer and set it to `NULL`.
 *
 * @param ptr Pointer with dynamically allocated resources for clean up.
 */
void pbcc_free_ptr(void** ptr);
#endif //PBCC_MEMORY_UTILS_H
