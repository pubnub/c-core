/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_get_native_socket.h"

#include "core/pubnub_log.h"
#include "pubnub_internal.h"

pbpal_native_socket_t pubnub_get_native_socket(pubnub_t* pb) {
    return pb->pal.socket;
}
