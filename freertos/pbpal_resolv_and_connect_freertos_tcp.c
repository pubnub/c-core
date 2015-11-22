/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbpal.h"

#include "pubnub_internal.h"
#include "pubnub_assert.h"


#define HTTP_PORT 80


enum pubnub_res pbpal_resolv_and_connect(pubnub_t *pb)
{
    struct freertos_sockaddr addr;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT((pb->state == PBS_IDLE) || (pb->state == PBS_WAIT_DNS));
    
    addr.sin_port = FreeRTOS_htons(HTTP_PORT);
    addr.sin_addr = FreeRTOS_gethostbyname(PUBNUB_ORIGIN_SETTABLE ? pb->origin : PUBNUB_ORIGIN);
    if (addr.sin_addr == 0) {
        return PNR_CONNECT_FAILED;
    }

    pb->pal.socket = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP);
    if (pb->pal.socket == SOCKET_INVALID) {
        return PNR_CONNECT_FAILED;
    }
    if (FreeRTOS_connect(pb->pal.socket, &addr, sizeof addr) != 0) {
        FreeRTOS_closesocket(pb->pal.socket);
        pb->pal.socket = SOCKET_INVALID;
        return PNR_CONNECT_FAILED;
    }

    {
        TickType_t tmval = pdMS_TO_TICKS(310 * 1000);
        FreeRTOS_setsockopt(pb->pal.socket, 0, FREERTOS_SO_RCVTIMEO, &tmval, sizeof tmval);
    }
    
    return PNR_STARTED;/* Should really be PNR_OK, see below */
    /* If we return PNR_OK, then the whole transaction can finish
       in one call to Netcore FSM. That would be nice, but some
       tests want to be able to cancel a request, which would
       then be impossible. So, until we figure out how to handle
       that, we shall return PNR_STARTED.
    */
}


enum pubnub_res pbpal_check_resolv_and_connect(pubnub_t *pb)
{
	/* should never be called! */
	PUBNUB_ASSERT_OPT(pb == NULL);
	return PNR_CONNECT_FAILED;
}