/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "lwip/sockets.h"
#include "lwip/ip_addr.h"
#include "lwip/netdb.h"
#include "lwip/inet.h"
#include "lwip/dns.h"
#include "pbpal.h"

#include "pubnub_internal.h"
#include "pubnub_assert.h"
#include "pubnub_log.h"
#include <string.h>


#define HTTP_PORT 80


enum pbpal_resolv_n_connect_result pbpal_resolv_and_connect(pubnub_t *pb)
{

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT((pb->state == PBS_READY) || (pb->state == PBS_WAIT_DNS_SEND)  || (pb->state == PBS_WAIT_DNS_RCV));

#if ESP_PLATFORM
    struct sockaddr_in addr;
    addr.sin_port = htons(HTTP_PORT);

    PUBNUB_LOG_TRACE("pbpal_resolv_and_connect: gethostbyname(%s)\n",
            PUBNUB_ORIGIN_SETTABLE ? pb->origin : PUBNUB_ORIGIN);

    struct hostent *host = gethostbyname(PUBNUB_ORIGIN_SETTABLE ? pb->origin : PUBNUB_ORIGIN);
    if (host == NULL) {
        PUBNUB_LOG_ERROR("pbpal_resolv_and_connect: getting host failed!\n");
        return pbpal_resolv_failed_processing;
    }
    addr.sin_addr = *((struct in_addr *)host->h_addr_list[0]);
    addr.sin_family = AF_INET;
    memcpy(&addr.sin_addr, host->h_addr_list[0], host->h_length);
    if (addr.sin_addr.s_addr == 0) {
        PUBNUB_LOG_ERROR("pbpal_resolv_and_connect: no address found!\n");
        return pbpal_resolv_failed_processing;
    }

    pb->pal.socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (pb->pal.socket == SOCKET_INVALID) {
        return pbpal_connect_resource_failure;
    }
    pbpal_set_tcp_keepalive(pb);
    if (connect(pb->pal.socket, (const struct sockaddr*) &addr, sizeof addr) != 0) {
        closesocket(pb->pal.socket);
        pb->pal.socket = SOCKET_INVALID;
        return pbpal_connect_failed;
    }
#else
    struct sockaddr addr;
    addr.sin_port = htons(HTTP_PORT);
    addr.sin_addr = gethostbyname(PUBNUB_ORIGIN_SETTABLE ? pb->origin : PUBNUB_ORIGIN);
    if (addr.sin_addr == 0) {
        return pbpal_resolv_failed_processing;
    }

    pb->pal.socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (pb->pal.socket == SOCKET_INVALID) {
        return pbpal_connect_resource_failure;
    }
    pbpal_set_tcp_keepalive(pb);
    if (connect(pb->pal.socket, &addr, sizeof addr) != 0) {
        closesocket(pb->pal.socket);
        pb->pal.socket = SOCKET_INVALID;
        return pbpal_connect_failed;
    }
#endif

    {
        TickType_t tmval = pdMS_TO_TICKS(pb->transaction_timeout_ms);
        setsockopt(pb->pal.socket, 0, SO_RCVTIMEO, &tmval, sizeof tmval);
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
    return (pb->flags.sent_queries < PUBNUB_MAX_DNS_QUERIES ? 0 : 1)
}
#endif /* PUBNUB_CHANGE_DNS_SERVERS */
#endif /* defined(PUBNUB_CALLBACK_API) */

void pbpal_set_tcp_keepalive(const pubnub_t *pb)
{
    if (pb->pal.socket == SOCKET_INVALID) return;
    const pubnub_tcp_keepalive keepalive = pb->options.tcp_keepalive;
    const pb_socket_t skt = pb->pal.socket;

    const int enabled = pbccTrue == keepalive.enabled ? 1 : 0;
    (void)setsockopt(skt, SOL_SOCKET, SO_KEEPALIVE, &enabled, sizeof(enabled));

    if (pbccTrue != keepalive.enabled ||
        (0 == keepalive.time &&  0 == keepalive.interval)) return;

    const int time = keepalive.time;

    if (time > 0) {
#if defined(TCP_KEEPIDLE)
        (void)setsockopt(skt, IPPROTO_TCP, TCP_KEEPIDLE, &time, sizeof(time));
#elif defined(TCP_KEEPALIVE)
        (void)setsockopt(skt, IPPROTO_TCP, TCP_KEEPALIVE, &time, sizeof(time));
#endif
    }

#if defined(TCP_KEEPINTVL)
    const int interval = keepalive.interval;
    if (interval > 0)
        (void)setsockopt(skt, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
#endif

#if defined(TCP_KEEPCNT)
    const int probes = keepalive.probes;
    if (probes > 0)
        (void)setsockopt(skt, IPPROTO_TCP, TCP_KEEPCNT, &probes, sizeof(probes));
#endif
}
