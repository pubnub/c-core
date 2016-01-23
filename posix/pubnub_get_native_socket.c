/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"


int pubnub_get_native_socket(pubnub_t *pb)
{
    return pb->pal.socket;
}
