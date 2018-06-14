/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if defined(_WIN32)
#include <winsock2.h>
#include <windows.h>
#endif

#include "pubnub_internal.h"
#include "core/pubnub_log.h"


pbpal_native_socket_t pubnub_get_native_socket(pubnub_t* pb)
{
    pbpal_native_socket_t fd = pb->pal.dns_socket;

    if (SOCKET_INVALID == fd) {
        fd = BIO_get_fd(pb->pal.socket, NULL);
        if (SOCKET_INVALID == fd) {
            PUBNUB_LOG_ERROR("Uninitialized BIO!\n");
        }
    }
    return fd;
}
