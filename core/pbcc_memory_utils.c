/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbcc_memory_utils.h"

#include <stdlib.h>


// ----------------------------------------------
//                   Functions
// ----------------------------------------------

void pbcc_free_ptr(void** ptr)
{
    if (NULL == ptr || NULL == *ptr) return;

    free(*ptr);
    *ptr = NULL;
}