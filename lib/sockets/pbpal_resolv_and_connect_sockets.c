/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "core/pbpal.h"

#include "pubnub_internal.h"
#include "core/pubnub_assert.h"
#include "core/pubnub_log.h"
#include "lib/sockets/pbpal_adns_sockets.h"
#include "core/pubnub_dns_servers.h"

#include <string.h>
#include <sys/types.h>

#if defined(_WIN32)
#include "windows/pubnub_get_native_socket.h"
#else
#include "posix/pubnub_get_native_socket.h"
#endif

#define HTTP_PORT 80

#define DNS_PORT 53

#define TLS_PORT 443

#ifndef PUBNUB_CALLBACK_API
#define send_dns_query(x,y,z) -1
#define read_response(x,y,z,v) -1
#endif


static void prepare_port_and_hostname(pubnub_t *pb, uint16_t* p_port, char const** p_origin)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT((pb->state == PBS_READY) || (pb->state == PBS_WAIT_DNS_SEND));
    *p_origin = PUBNUB_ORIGIN_SETTABLE ? pb->origin : PUBNUB_ORIGIN;
#if PUBNUB_USE_SSL
    if (pb->flags.trySSL) {
        PUBNUB_ASSERT(pb->options.useSSL);
        *p_port = TLS_PORT;
    }
#endif
#if PUBNUB_PROXY_API
    switch (pb->proxy_type) {
    case pbproxyHTTP_CONNECT:
        if (!pb->proxy_tunnel_established) {
            *p_origin = pb->proxy_hostname;
        }
        *p_port = pb->proxy_port;
        break;
    case pbproxyHTTP_GET:
        *p_origin = pb->proxy_hostname;
        *p_port = pb->proxy_port;
        PUBNUB_LOG_TRACE("Using proxy: %s : %hu\n", *p_origin, *p_port);
        break;
    default:
        break;
    }
#endif
    return;
}

#ifdef PUBNUB_CALLBACK_API
static void get_dns_ip(struct sockaddr_in* addr)
{
    void* p = &(addr->sin_addr.s_addr);
    if((pubnub_get_dns_primary_server_ipv4((struct pubnub_ipv4_address*)p) == -1)
       && (pubnub_get_dns_secondary_server_ipv4((struct pubnub_ipv4_address*)p) == -1)) {
        inet_pton(AF_INET, PUBNUB_DEFAULT_DNS_SERVER, p);
    }
}

static enum pbpal_resolv_n_connect_result connect_TCP_socket(pubnub_t *pb,
                                                             struct sockaddr_in dest,
                                                             const uint16_t port)
{
    pb->pal.socket  = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (SOCKET_INVALID == pb->pal.socket) {

        return pbpal_connect_resource_failure;
    }
    pb->options.use_blocking_io = false;
    pbpal_set_blocking_io(pb);
    socket_disable_SIGPIPE(pb->pal.socket);
    dest.sin_port = htons(port);
    if (SOCKET_ERROR == connect(pb->pal.socket, (struct sockaddr*)&dest, sizeof dest)) {
        return socket_would_block() ? pbpal_connect_wouldblock : pbpal_connect_failed;
    }

    return pbpal_connect_success;
}
#endif

enum pbpal_resolv_n_connect_result pbpal_resolv_and_connect(pubnub_t *pb)
{
    int error;
    uint16_t port = HTTP_PORT;
    char const* origin;

#ifdef PUBNUB_CALLBACK_API
    struct sockaddr_in dest;

    prepare_port_and_hostname(pb, &port, &origin);
#if PUBNUB_PROXY_API
    if (0 != pb->proxy_ip_address.ipv4[0]) {
        struct pubnub_ipv4_address* p_dest_addr = (struct pubnub_ipv4_address*)&(dest.sin_addr.s_addr);
        memset(&dest, '\0', sizeof dest);
        memcpy(p_dest_addr->ipv4, pb->proxy_ip_address.ipv4, sizeof p_dest_addr->ipv4);
        dest.sin_family = AF_INET;

        return connect_TCP_socket(pb, dest, port);
    }
#endif

    if (SOCKET_INVALID == pb->pal.socket) {
        pb->pal.socket  = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    }
    if (SOCKET_INVALID == pb->pal.socket) {
        return pbpal_resolv_resource_failure;
    }
    pb->options.use_blocking_io = false;
    pbpal_set_blocking_io(pb);
    dest.sin_family = AF_INET;
    dest.sin_port = htons(DNS_PORT);
    get_dns_ip(&dest);
    error = send_dns_query(pb->pal.socket, (struct sockaddr*)&dest, origin);
    if (error < 0) {
        return pbpal_resolv_failed_send;
    }
    else if (error > 0) {
        return pbpal_resolv_send_wouldblock;
    }
    return pbpal_resolv_sent;

#else
    char port_string[20];
    struct addrinfo *result;
    struct addrinfo *it;
    struct addrinfo hint;

    hint.ai_socktype = SOCK_STREAM;
    hint.ai_family = AF_UNSPEC;
    hint.ai_protocol = hint.ai_flags = hint.ai_addrlen = 0;
    hint.ai_addr = NULL;
    hint.ai_canonname = NULL;
    hint.ai_next = NULL;

    prepare_port_and_hostname(pb, &port, &origin);
    snprintf(port_string, sizeof port_string, "%hu", port);
    error = getaddrinfo(origin, port_string, &hint, &result);
    if (error != 0) {
        return pbpal_resolv_failed_processing;
    }

    for (it = result; it != NULL; it = it->ai_next) {
        pb->pal.socket = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
        if (pb->pal.socket == SOCKET_INVALID) {
            continue;
        }
        pbpal_set_blocking_io(pb);
        if (connect(pb->pal.socket, it->ai_addr, it->ai_addrlen) == SOCKET_ERROR) {
            if (socket_would_block()) {
                error = 1;
                break;
            }
            else {
                PUBNUB_LOG_WARNING("socket connect() failed, will try another IP address, if available\n");
                socket_close(pb->pal.socket);
                pb->pal.socket = SOCKET_INVALID;
                continue;
            }
        }
        break;
    }
    freeaddrinfo(result);

    if (NULL == it) {
        return pbpal_connect_failed;
    }

    socket_set_rcv_timeout(pb->pal.socket, pb->transaction_timeout_ms);
    socket_disable_SIGPIPE(pb->pal.socket);

    return error ? pbpal_connect_wouldblock : pbpal_connect_success;
#endif /* PUBNUB_CALLBACK_API */
}


enum pbpal_resolv_n_connect_result pbpal_check_resolv_and_connect(pubnub_t *pb)
{
#ifdef PUBNUB_CALLBACK_API

    struct sockaddr_in dns_server;
    struct sockaddr_in dest;
    uint16_t port = HTTP_PORT;
    pbpal_native_socket_t skt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT(pb->state == PBS_WAIT_DNS_RCV);
#if PUBNUB_USE_SSL
    if (pb->flags.trySSL) {
        PUBNUB_ASSERT(pb->options.useSSL);
        port = TLS_PORT;
    }
#endif
#if PUBNUB_PROXY_API
    if (pbproxyNONE != pb->proxy_type) {
        port = pb->proxy_port;
    }
#endif
    skt = pb->pal.socket;
    PUBNUB_ASSERT(SOCKET_INVALID != skt);

    dns_server.sin_family = AF_INET;
    dns_server.sin_port = htons(DNS_PORT);
    get_dns_ip(&dns_server);
    memset(&dest, '\0', sizeof dest);
    switch (read_dns_response(skt, (struct sockaddr*)&dns_server, &dest)) {
    case -1:
		return pbpal_resolv_failed_rcv;
    case +1:
        return pbpal_resolv_rcv_wouldblock;
    case 0:
        break;
    }
    socket_close(skt);

    return connect_TCP_socket(pb, dest, port);
#else

    PUBNUB_UNUSED(pb);

    /* Under regular BSD-ish sockets, this function should not be
       called unless using async DNS, so this is an error */
    return pbpal_connect_failed;

#endif /* PUBNUB_CALLBACK_API */
}


enum pbpal_resolv_n_connect_result pbpal_check_connect(pubnub_t *pb)
{
    fd_set write_set;
    int rslt;
    struct timeval timev = { 0, 300000 };

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT(pb->state == PBS_WAIT_CONNECT);

    FD_ZERO(&write_set);
    FD_SET(pb->pal.socket, &write_set);
    rslt = select(pb->pal.socket + 1, NULL, &write_set, NULL, &timev);
    if (SOCKET_ERROR == rslt) {
        PUBNUB_LOG_ERROR("pbpal_connected(): select() Error!\n");
        return pbpal_connect_resource_failure;
    }
    else if (rslt > 0) {
        PUBNUB_LOG_TRACE("pbpal_connected(): select() event\n");
        return pbpal_connect_success;
    }
    PUBNUB_LOG_TRACE("pbpal_connected(): no select() events\n");
    return pbpal_connect_wouldblock;
}
