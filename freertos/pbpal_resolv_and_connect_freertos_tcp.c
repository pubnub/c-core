/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbpal.h"

#include "pubnub_internal.h"
#include "pubnub_assert.h"


#define HTTP_PORT 80


enum pbpal_resolv_n_connect_result pbpal_resolv_and_connect(pubnub_t *pb)
{
    struct freertos_sockaddr addr;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT((pb->state == PBS_READY) || (pb->state == PBS_WAIT_DNS_SEND)  || (pb->state == PBS_WAIT_DNS_RCV));
    
    addr.sin_port = FreeRTOS_htons(HTTP_PORT);
    addr.sin_addr = FreeRTOS_gethostbyname(PUBNUB_ORIGIN_SETTABLE ? pb->origin : PUBNUB_ORIGIN);
    if (addr.sin_addr == 0) {
        return pbpal_resolv_failed_processing;
    }

    pb->pal.socket = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP);
    if (pb->pal.socket == SOCKET_INVALID) {
        return pbpal_connect_resource_failure;
    }
    if (FreeRTOS_connect(pb->pal.socket, &addr, sizeof addr) != 0) {
        FreeRTOS_closesocket(pb->pal.socket);
        pb->pal.socket = SOCKET_INVALID;
        return pbpal_connect_failed;
    }

    {
        TickType_t tmval = pdMS_TO_TICKS(pb->transaction_timeout_ms);
        FreeRTOS_setsockopt(pb->pal.socket, 0, FREERTOS_SO_RCVTIMEO, &tmval, sizeof tmval);
    }

    return pbpal_connect_success;
}


enum pbpal_resolv_n_connect_result pbpal_check_resolv_and_connect(pubnub_t *pb)
{
    /* should never be called, as FreeRTOS+TCP is always blocking */
    PUBNUB_ASSERT_OPT(pb == NULL);
    return pbpal_connect_resource_failure;
}



enum pbpal_resolv_n_connect_result pbpal_check_connect(pubnub_t *pb)
{
    /* should never be called, as FreeRTOS+TCP is always blocking */
    PUBNUB_ASSERT_OPT(pb == NULL);
    return pbpal_connect_resource_failure;
}

#if defined(PUBNUB_CALLBACK_API)
#if PUBNUB_CHANGE_DNS_SERVERS
int pbpal_dns_rotate_server(pubnub_t *pb)
{
    return (pbp->flags.sent_queries < PUBNUB_MAX_DNS_QUERIES ? 0 : 1)
}
#endif /* PUBNUB_CHANGE_DNS_SERVERS */
#endif /* defined(PUBNUB_CALLBACK_API) */