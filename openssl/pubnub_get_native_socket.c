/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include <winsock2.h>
#include <windows.h>

#include "pubnub_get_native_socket.h"

#if defined(_WIN32)
SOCKET pubnub_get_native_socket(pubnub_t *pb) { SOCKET socket;
#else
int pubnub_get_native_socket(pubnub_t *pb) { int socket;
#endif
    if (-1 == BIO_get_fd(pb->pal.socket, &socket)) {
        DEBUG_PRINTF("Uninitialized BIO!\n");
        return -1;
    }
    return socket;
}
