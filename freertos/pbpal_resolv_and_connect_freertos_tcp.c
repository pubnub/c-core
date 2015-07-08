/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbpal.h"

#define FREERTOS
#include "pubnub_internal.h"
#include "pubnub_assert.h"

#include "FreeRTOS_sockets.h"


#define HTTP_PORT 80


int pbpal_resolv_and_connect(pubnub_t *pb)
{
    struct freertos_sockaddr addr;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT((pb->state == PBS_IDLE) || (pb->state == PBS_WAIT_DNS));
    
    addr.sin_port = FreeRTOS_htons(HTTP_PORT);
    addr.sin_addr = FreeRTOS_gethostbyname(PUBNUB_ORIGIN);
    if (addr.sin_addr == 0) {
        return -1;
    }

    pb->socket = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP);
    if (pb->socket == FREERTOS_INVALID_SOCKET) {
        return -1;
    }
    if (FreeRTOS_connect(pb->socket, &addr, sizeof addr) != 0) {
        FreeRTOS_closesocket(pb->socket);
        pb->socket = -1;
        return -1;
    }
    
    return 0;
}
