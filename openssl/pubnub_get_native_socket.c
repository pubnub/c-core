/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if defined(_WIN32)
#include <winsock2.h>
#include <windows.h>

#include "pubnub_get_native_socket.h"
#include "pubnub_log.h"

SOCKET pubnub_get_native_socket(pubnub_t *pb) { SOCKET socket;

#else
#include "pubnub_get_native_socket.h"
#include "pubnub_log.h"

int pubnub_get_native_socket(pubnub_t *pb) { int socket;
#endif
    if (-1 == BIO_get_fd(pb->pal.socket, &socket)) {
        PUBNUB_LOG_ERROR("Uninitialized BIO!\n");
        return -1;
    }
    return socket;
}
