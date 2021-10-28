/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PB_STRNLEN_S
#define INC_PB_STRNLEN_S

#include <stddef.h>

/** Returns the length of the given null-terminated byte string, that is, the number
    of characters in a character array whose first element is pointed to by @p str up to
    and not including the first null character.
    The function returns zero if @p str is a null pointer and returns @p strsz if the null
    character was not found in the first @p strsz bytes of @p str.
    The behavior is undefined if both @p str points to a character array which lacks the null
    character and the size of that character array is less than @p strsz; in other words,
    an erroneous value of @p strsz does not expose the impending buffer overflow.
    @param str pointer to the null-terminated byte string to be examined.
    @param strsz maximum number of characters to examine.
  */
size_t pb_strnlen_s(const char *str, size_t strsz);

#endif /* INC_PB_STRNLEN_S */
