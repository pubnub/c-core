/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PB_STRNCASECMP
#define INC_PB_STRNCASECMP

#include <stddef.h>

/** The strcasecmp() function compares string1 and string2 without sensitivity to case
    in given first characters.
    All alphabetic characters in string1 and string2 are converted to lowercase before comparison.
 
    @param str1 pointer to the null-terminated byte string to be compared.
    @param str2 pointer to the null-terminated byte string to be compared.
    @param number_of_chars number of characters to compare.
  */
int pb_strncasecmp(const char *str1, const char *str2, size_t number_of_chars);

#endif /* INC_PB_STRNLEN_S */
