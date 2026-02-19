/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "core/pbpal.h"
#include "lib/sockets/pbpal_socket_blocking_io.h"

#include "core/pubnub_assert.h"

#if PUBNUB_USE_SSL
#include "openssl/pubnub_internal.h"
#else
#include "pubnub_internal.h"
#endif

#include <windows.h>


int socket_platform_init(void)
{
    WSADATA wsadata;
    return WSAStartup(MAKEWORD(2, 2), &wsadata);
}


int pbpal_set_blocking_io(pubnub_t* pb)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    return pbpal_set_socket_blocking_io(
        pb, pb->pal.socket, pb->options.use_blocking_io);
}
