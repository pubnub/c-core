/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbpal.h"

#include "pubnub_internal.h"
#include "pubnub_assert.h"


#define HTTP_PORT 80

#define HTTPS_PORT 443


enum pbpal_resolv_n_connect_result pbpal_resolv_and_connect(pubnub_t *pb)
{
    char const* origin = PUBNUB_ORIGIN_SETTABLE ? pb->origin : PUBNUB_ORIGIN;
#if PUBNUB_USE_IPV6
    TCPIP_DNS_RESOLVE_TYPE query_type =
        pb->options.ipv6_connectivity ? TCPIP_DNS_TYPE_AAAA : TCPIP_DNS_TYPE_A;
#else
    TCPIP_DNS_RESOLVE_TYPE query_type = TCPIP_DNS_TYPE_A;
#endif
    TCPIP_DNS_RESULT dns_result = TCPIP_DNS_Resolve(origin, query_type);
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT((pb->state == PBS_IDLE) || (pb->state == PBS_WAIT_DNS_SEND) 
            || (pb->state == PBS_WAIT_DNS_RCV));
    
    switch (dns_result) {
        case TCPIP_DNS_RES_OK:
        case TCPIP_DNS_RES_NAME_IS_IPADDRESS:
            return pbpal_check_resolv_and_connect(pb);
        case TCPIP_DNS_RES_SOCKET_ERROR:
            /* it seems that this might be "would block" sometimes, but not
             * sure how to go about figuring out the distinction
             */
            return pbpal_resolv_failed_send; 
        case TCPIP_DNS_RES_PENDING:
            return pbpal_resolv_rcv_wouldblock;
        case TCPIP_DNS_RES_SERVER_TMO:
        case TCPIP_DNS_RES_NO_INTERFACE:
            return pbpal_resolv_send_wouldblock;
        default:
            return pbpal_resolv_resource_failure;
    }
}


enum pbpal_resolv_n_connect_result pbpal_check_resolv_and_connect(pubnub_t *pb)
{
    char const* origin = PUBNUB_ORIGIN_SETTABLE ? pb->origin : PUBNUB_ORIGIN;
#if PUBNUB_USE_IPV6
    IP_ADDRESS_TYPE address_type =
        pb->options.ipv6_connectivity ? IP_ADDRESS_TYPE_IPV6 : IP_ADDRESS_TYPE_IPV4;
#else
    IP_ADDRESS_TYPE address_type = IP_ADDRESS_TYPE_IPV4;
#endif
    IP_MULTI_ADDRESS ip_addr;
    TCPIP_DNS_RESULT dns_result = TCPIP_DNS_IsResolved(origin, &ip_addr, address_type);
	PUBNUB_ASSERT_OPT(pb != NULL);
    switch (dns_result) {
        case TCPIP_DNS_RES_OK:
            if (SOCKET_INVALID == pb->pal.socket) {
#if PUBNUB_USE_SSL            
                pb->pal.socket = NET_PRES_SocketOpen(
                        0, 
                        pb->options.useSSL ? NET_PRES_SKT_ENCRYPTED_STREAM_CLIENT : NET_PRES_SKT_UNENCRYPTED_STREAM_CLIENT,
                        address_type,
                        HTTPS_PORT, 
                        (NET_PRES_ADDRESS*)&ip_addr, NULL
                        );
#else
                pb->pal.socket = TCPIP_TCP_ClientOpen(address_type, HTTP_PORT, &ip_addr);
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


enum pbpal_resolv_n_connect_result pbpal_check_connect(pubnub_t *pb)
{
#if PUBNUB_USE_SSL
    bool connected = !NET_PRES_SocketIsNegotiatingEncryption(pb->pal.socket) &&
            NET_PRES_SocketIsSecure(pb->pal.socket);
    return bool ? pbpal_connect_success : pbpal_connect_wouldblock;
#else
    return TCPIP_TCP_IsConnected(pb->pal.socket) ? pbpal_connect_success : pbpal_connect_wouldblock;
#endif
}

#if defined(PUBNUB_CALLBACK_API)
#if PUBNUB_CHANGE_DNS_SERVERS
int pbpal_dns_rotate_server(pubnub_t *pb)
{
    return (pbp->flags.sent_queries < PUBNUB_MAX_DNS_QUERIES ? 0 : 1)
}
#endif /* PUBNUB_CHANGE_DNS_SERVERS */
#endif /* defined(PUBNUB_CALLBACK_API) */