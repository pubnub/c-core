/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "lib/pb_strnlen_s.h"

size_t pb_strnlen_s(const char *str, size_t strsz)
{
    size_t i;

    if (NULL == str) {
        return 0;
    }
    for(i = 0; i < strsz; i++, str++) {
        if ('\0' == *str) {
            break;
        }
    }
    
    return i;
}
