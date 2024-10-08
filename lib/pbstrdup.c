/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbstrdup.h"

#include <stdbool.h>
#include <string.h>


// ----------------------------------
//        Function prototypes
// ----------------------------------

/**
 * @brief Create a copy of specific length from provided byte-string.
 *
 * @param src       Pointer to the null-terminated byte string to copy.
 * @param len       How many (at most) bytes from `src` should be written into
 *                  copy.
 * @param check_len Whether `len` should be checked to not be larger that `src`
 *                  byte string length.
 * @return Pointer to the null-terminated byte string which is copied from
 *         `src`. `NULL` value will be returned in case of insufficient memory
 *         error.
 */
static char* _pbstrndup(const char* src, size_t len, bool check_len);


// ----------------------------------
//             Functions
// ----------------------------------

char* pbstrdup(const char* src)
{
    return _pbstrndup(src, strlen(src), false);
}

char* pbstrndup(const char* src, const size_t len)
{
    return _pbstrndup(src, len, true);
}

char* _pbstrndup(const char* src, size_t len, const bool check_len)
{
    if (check_len) {
        const size_t actual_len = strlen(src);
        if (actual_len < len) { len = actual_len; }
    }

    char* copy = malloc(len + 1);
    if (NULL != copy) {
        memcpy(copy, src, len);
        copy[len] = '\0';
    }

    return copy;
}