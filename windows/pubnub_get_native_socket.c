/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if PUBNUB_USE_SSL
#include "openssl/pubnub_internal.h"
#else
#include "pubnub_internal.h"
#endif

#include "pubnub_get_native_socket.h"
#include "core/pubnub_log.h"

pbpal_native_socket_t pubnub_get_native_socket(pubnub_t *pb)
{
    return pb->pal.socket;
}
