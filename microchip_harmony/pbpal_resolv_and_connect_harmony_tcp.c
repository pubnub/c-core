/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbpal.h"

#include "pubnub_internal.h"
#include "pubnub_assert.h"


#define HTTP_PORT 80

#define HTTPS_PORT 443


enum pbpal_resolv_n_connect_result pbpal_resolv_and_connect(pubnub_t *pb)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT((pb->state == PBS_IDLE) || (pb->state == PBS_WAIT_DNS));
    
    switch (TCPIP_DNS_Resolve(PUBNUB_ORIGIN_SETTABLE ? pb->origin : PUBNUB_ORIGIN, TCPIP_DNS_TYPE_A)) {
        case TCPIP_DNS_RES_OK:
        case TCPIP_DNS_RES_NAME_IS_IPADDRESS:
            return pbpal_check_resolv_and_connect(pb);
        case TCPIP_DNS_RES_SOCKET_ERROR:
            /* it seems that this might be "would block" sometimes, but not
             * sure how to go about figuring out the distinction
             */
            return pbpal_resolv_failed_send; 
        case TCPIP_DNS_RES_PENDING:
            /* It might not be the send, but it doesn't matter */
            return pbpal_resolv_send_wouldblock;
        default:
            return pbpal_resolv_resource_failure;
    }
}


enum pbpal_resolv_n_connect_result pbpal_check_resolv_and_connect(pubnub_t *pb)
{
    char const* origin = PUBNUB_ORIGIN_SETTABLE ? pb->origin : PUBNUB_ORIGIN;
    IP_MULTI_ADDRESS ip_addr;
	PUBNUB_ASSERT_OPT(pb == NULL);

    switch (TCPIP_DNS_IsResolved(origin, &ip_addr, IP_ADDRESS_TYPE_IPV4)) {
        case TCPIP_DNS_RES_OK:
            if (SOCKET_INVALID == pb->pal.socket) {
#if PUBNUB_USE_SSL            
                pb->pal.socket = NET_PRES_SocketOpen(
                        0, 
                        pb->options.useSSL ? NET_PRES_SKT_ENCRYPTED_STREAM_CLIENT : NET_PRES_SKT_UNENCRYPTED_STREAM_CLIENT,
                        IP_ADDRESS_TYPE_IPV4, 
                        HTTPS_PORT, 
                        (NET_PRES_ADDRESS*)&ip_addr, NULL
                        );
#else
                pb->pal.socket = TCPIP_TCP_ClientOpen(IP_ADDRESS_TYPE_IPV4, HTTP_PORT, &ip_addr);
#endif
            }
            if (SOCKET_INVALID == pb->pal.socket) {
                return pbpal_connect_resource_failure;
            }
            /* It's not clear that failure here means "you can retry" if using
             * `NET_PRES_` functions...
             */
            return socket_connect(pb->pal.socket) ? pbpal_connect_failed : pbpal_connect_wouldblock;
        case TCPIP_DNS_RES_PENDING:
            return pbpal_resolv_rcv_wouldblock;
            break;
        case TCPIP_DNS_RES_NAME_IS_IPADDRESS:
            return pbpal_resolv_failed_processing;
        default:
            return pbpal_resolv_resource_failure;
    }
}


bool pbpal_connected(pubnub_t *pb)
{
#if PUBNUB_USE_SSL
    return !NET_PRES_SocketIsNegotiatingEncryption(pb->pal.socket) &&
            NET_PRES_SocketIsSecure(pb->pal.socket);
#else
    return TCPIP_TCP_IsConnected(pb->pal.socket);
#endif
}