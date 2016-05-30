/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if defined(_WIN32)
#include <winsock2.h>
#include <windows.h>
#endif

#include "pubnub_get_native_socket.h"
#include "pubnub_log.h"

pbpal_native_socket_t pubnub_get_native_socket(pubnub_t *pb)
{
    pbpal_native_socket_t fd = BIO_get_fd(pb->pal.socket, NULL);
    if (-1 == fd) {
        PUBNUB_LOG_ERROR("Uninitialized BIO!\n");
    }
    return fd;
}
