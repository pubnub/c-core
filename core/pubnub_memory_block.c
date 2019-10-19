/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"
#include "pubnub_memory_block.h"
#include "lib/pb_strnlen_s.h"

pubnub_chamebl_t pubnub_str_2_chamebl_t(char* str)
{
    pubnub_chamebl_t rslt;
    rslt.ptr = str;
    rslt.size = (NULL == str) ? 0 : pb_strnlen_s(str, PUBNUB_MAX_OBJECT_LENGTH);
    return rslt;
}
