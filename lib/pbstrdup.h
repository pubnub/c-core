/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#ifndef PBSTRDUP_H
#define PBSTRDUP_H

#include <stdlib.h>

/**
 * @file pbstrdup.h
 * @brief Portable `strdup` implementation.
 */

/**
 * @brief Create a copy from provided byte-string.
 *
 * @param src Pointer to the null-terminated byte string to copy.
 * @return Pointer to the null-terminated byte string which is copied from
 *         `src`. `NULL` value will be returned in case of insufficient memory
 *         error.
 */
char* pbstrdup(const char* src);

/**
 * @brief Create a copy of specific length from provided byte-string.
 *
 * @note Of `len` is larger than `src` actual length then it will be ignored.
 *
 * @param src Pointer to the null-terminated byte string to copy.
 * @param len How many (at most) bytes from `src` should be written into copy.
 * @return Pointer to the null-terminated byte string which is copied from
 *         `src`. `NULL` value will be returned in case of insufficient memory
 *         error.
 */
char* pbstrndup(const char* src, size_t len);
#endif //PBSTRDUP_H