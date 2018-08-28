/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"
#include "pubnub_get_native_socket.h"

pbpal_native_socket_t pubnub_get_native_socket(pubnub_t* pb)
{
    return pb->pal.socket;
}
