/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#include "core/pbpal.h"
#include "core/pubnub_assert.h"
#include "core/pubnub_log.h"
#include "lib/sockets/pbpal_adns_sockets.h"
#include "lib/sockets/pbpal_socket_blocking_io.h"
#ifdef PUBNUB_NTF_RUNTIME_SELECTION
#include "pubnub_ntf_enforcement.h"
#endif

#include <string.h>
#include <sys/types.h>

#if defined(_WIN32)
#include "windows/pubnub_get_native_socket.h"
#else
#include "posix/pubnub_get_native_socket.h"
#endif

#define HTTP_PORT 80

#define TLS_PORT 443

#ifndef PUBNUB_CALLBACK_API
#define send_dns_query(x, y, z, v) -1
#define read_response(x, y, z, v) -1
#else
/** Considering the size of bit field for DNS queries sent */
PUBNUB_STATIC_ASSERT(PUBNUB_MAX_DNS_QUERIES < (1 << SENT_QUERIES_SIZE_IN_BITS),
                     PUBNUB_MAX_DNS_QUERIES_is_too_big);
#if PUBNUB_CHANGE_DNS_SERVERS
/** Considering the size of bit field for DNS servers rotations count */
PUBNUB_STATIC_ASSERT(PUBNUB_MAX_DNS_ROTATION < (1 << ROTATIONS_COUNT_SIZE_IN_BITS),
                     PUBNUB_MAX_DNS_ROTATION_is_too_big);
#endif /* PUBNUB_CHANGE_DNS_SERVERS */
#if PUBNUB_USE_IPV6
typedef struct sockaddr_storage sockaddr_inX_t;
#define QUERY_TYPE pb->options.ipv6_connectivity ? dnsAAAA : dnsA
#else
typedef struct sockaddr_in sockaddr_inX_t;
#define QUERY_TYPE dnsA
#endif
#endif /* PUBNUB_CALLBACK_API */


static void prepare_port_and_hostname(pubnub_t*    pb,
                                      uint16_t*    p_port,
                                      char const** p_origin)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT((pb->state == PBS_READY) || (pb->state == PBS_WAIT_DNS_SEND));
    *p_origin = PUBNUB_ORIGIN;
    *p_port = HTTP_PORT;
#if PUBNUB_USE_SSL
    if (pb->flags.trySSL) {
        PUBNUB_ASSERT(pb->options.useSSL);
        *p_port = TLS_PORT;
    }
#endif
#if PUBNUB_ORIGIN_SETTABLE
    if (pb->port != INITIAL_PORT_VALUE) {
        *p_port = pb->port;
    }
    *p_origin = pb->origin;
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
        *p_port   = pb->proxy_port;
        PUBNUB_LOG_TRACE("Using proxy: %s : %hu\n", *p_origin, *p_port);
        break;
    default:
        break;
    }
#endif
    return;
}


#ifdef PUBNUB_CALLBACK_API

static void get_default_dns_ip(struct pubnub_ipv4_address *addr) {
    if (pubnub_dns_read_system_servers_ipv4(addr, 1) != 1) {
        inet_pton(AF_INET, PUBNUB_DEFAULT_DNS_SERVER, addr);
    }
}

#if PUBNUB_SET_DNS_SERVERS
#if PUBNUB_CHANGE_DNS_SERVERS
static void get_dns_ip(struct pbdns_servers_check* dns_check, struct sockaddr* addr)
{
    void* p = &(((struct sockaddr_in*)addr)->sin_addr.s_addr);
#if PUBNUB_USE_IPV6
    void* pv6 = ((struct sockaddr_in6*)addr)->sin6_addr.s6_addr;
#endif
    addr->sa_family     = AF_INET;
    dns_check->dns_mask = 1;
    if ((pubnub_get_dns_primary_server_ipv4((struct pubnub_ipv4_address*)p) == -1)
        || (dns_check->dns_server_check & dns_check->dns_mask)) {
        dns_check->dns_mask <<= 1;
        if ((pubnub_get_dns_secondary_server_ipv4((struct pubnub_ipv4_address*)p)
             == -1)
            || (dns_check->dns_server_check & dns_check->dns_mask)) {
            dns_check->dns_mask <<= 1;
#if PUBNUB_USE_IPV6
            addr->sa_family = AF_INET6;
#else
            get_default_dns_ip((struct pubnub_ipv4_address*)p);
#endif /* PUBNUB_USE_IPV6 */
        }
    }
#if PUBNUB_USE_IPV6
    if (AF_INET6 == addr->sa_family) {
        if ((pubnub_get_dns_primary_server_ipv6((struct pubnub_ipv6_address*)pv6)
             == -1)
            || (dns_check->dns_server_check & dns_check->dns_mask)) {
            dns_check->dns_mask <<= 1;
            if ((pubnub_get_dns_secondary_server_ipv6((struct pubnub_ipv6_address*)pv6)
                 == -1)
                || (dns_check->dns_server_check & dns_check->dns_mask)) {
                dns_check->dns_mask <<= 1;
                addr->sa_family = AF_INET;
                get_default_dns_ip((struct pubnub_ipv4_address*)p);
            }
        }
    }
#endif /* PUBNUB_USE_IPV6 */
}
#else
static void get_dns_ip(struct sockaddr* addr)
{
    void* p         = &(((struct sockaddr_in*)addr)->sin_addr.s_addr);
#if PUBNUB_USE_IPV6
    void* pv6       = ((struct sockaddr_in6*)addr)->sin6_addr.s6_addr;
#endif
    addr->sa_family = AF_INET;
    if ((pubnub_get_dns_primary_server_ipv4((struct pubnub_ipv4_address*)p) == -1)
        && (pubnub_get_dns_secondary_server_ipv4((struct pubnub_ipv4_address*)p)
            == -1)) {
#if PUBNUB_USE_IPV6
        addr->sa_family = AF_INET6;
#else
        get_default_dns_ip((struct pubnub_ipv4_address*)p);
#endif /* PUBNUB_USE_IPV6 */
    }
#if PUBNUB_USE_IPV6
    if (AF_INET6 == addr->sa_family) {
        if ((pubnub_get_dns_primary_server_ipv6((struct pubnub_ipv6_address*)pv6)
             == -1)
            && (pubnub_get_dns_secondary_server_ipv6((struct pubnub_ipv6_address*)pv6)
                == -1)) {
            addr->sa_family = AF_INET;
            get_default_dns_ip((struct pubnub_ipv4_address*)p);
        }
    }
#endif
}
#endif /* PUBNUB_CHANGE_DNS_SERVERS */
#else
static void get_dns_ip(struct sockaddr* addr)
{
    addr->sa_family = AF_INET;
    struct pubnub_ipv4_address* p = (struct pubnub_ipv4_address*)&(((struct sockaddr_in*)addr)->sin_addr.s_addr);
    get_default_dns_ip(p);
}
#endif /* PUBNUB_SET_DNS_SERVERS */


static enum pbpal_resolv_n_connect_result
connect_TCP_socket(pb_socket_t*           skt,
                   struct pubnub_options* options,
                   struct sockaddr*       dest,
                   const uint16_t         port)
{
    size_t sockaddr_size;

    PUBNUB_ASSERT_OPT(dest != NULL);

    switch (dest->sa_family) {
    case AF_INET:
        sockaddr_size                         = sizeof(struct sockaddr_in);
        ((struct sockaddr_in*)dest)->sin_port = htons(port);
        PUBNUB_LOG_TRACE("connect_TCP_socket(Ipv4:%s)\n",
                         inet_ntoa(((struct sockaddr_in*)dest)->sin_addr));
        break;
#if PUBNUB_USE_IPV6
    case AF_INET6: {
        char str[INET6_ADDRSTRLEN];
        sockaddr_size                           = sizeof(struct sockaddr_in6);
        ((struct sockaddr_in6*)dest)->sin6_port = htons(port);
        PUBNUB_LOG_TRACE("connect_TCP_socket(Ipv6:%s)\n",
                         inet_ntop(AF_INET6,
                                   &(((struct sockaddr_in6*)dest)->sin6_addr),
                                   str,
                                   INET6_ADDRSTRLEN));
    } break;
#endif
    default:
        PUBNUB_LOG_ERROR(
            "connect_TCP_socket(socket=%ld): invalid internet protokol "
            "dest->sa_family =%uh\n",
            (long)*skt,
            dest->sa_family);
        return pbpal_connect_failed;
    }
    *skt = socket(dest->sa_family, SOCK_STREAM, IPPROTO_TCP);
    if (SOCKET_INVALID == *skt) {
        return pbpal_connect_resource_failure;
    }
    options->use_blocking_io = false;
    pbpal_set_socket_blocking_io(*skt, options->use_blocking_io);
    socket_disable_SIGPIPE(*skt);
    if (SOCKET_ERROR == connect(*skt, dest, sockaddr_size)) {
        return socket_would_block() ? pbpal_connect_wouldblock
                                    : pbpal_connect_failed;
    }
    return pbpal_connect_success;
}


#if PUBNUB_ADNS_RETRY_AFTER_CLOSE
static void if_no_retry_close_socket(pb_socket_t* skt, struct pubnub_flags* flags)
{
    if (!flags->retry_after_close && (*skt != SOCKET_INVALID)) {
        socket_close(*skt);
        *skt = SOCKET_INVALID;
    }
}
#endif /* PUBNUB_ADNS_RETRY_AFTER_CLOSE */


#if PUBNUB_CHANGE_DNS_SERVERS
int pbpal_dns_rotate_server(pubnub_t *pb)
{
    struct pbdns_servers_check* dns_check = &pb->dns_check;
    struct pubnub_flags* flags = &pb->flags;
    dns_check->dns_server_check |= dns_check->dns_mask;

    if ((dns_check->dns_mask >= PUBNUB_MAX_DNS_SERVERS_MASK)
        && (flags->rotations_count < PUBNUB_MAX_DNS_ROTATION)) 
    {
        dns_check->dns_server_check = 0;
        /** Update how many times all DNS servers has been tried to process query */
        flags->rotations_count++;
    }
    
    if (flags->rotations_count >= PUBNUB_MAX_DNS_ROTATION) {
        flags->retry_after_close = false;
        flags->rotations_count = 1;
        return 1;
    }
    
    /** Going with new DNS server, after retry, brings new set of queries */
    flags->sent_queries = 0;

    return 0;
}

static void check_dns_server_error(struct pbdns_servers_check* dns_check,
                                   struct pubnub_flags*        flags)
{
    /** We just checked(notified) error with current DNS server */
    dns_check->dns_server_check |= dns_check->dns_mask;
    if (dns_check->dns_mask < PUBNUB_MAX_DNS_SERVERS_MASK) {
        /** Going with new DNS server, after retry, brings new set of queries */
        flags->sent_queries = 0;
        flags->retry_after_close = true;
    }
}
#endif /* PUBNUB_CHANGE_DNS_SERVERS */

#if PUBNUB_USE_MULTIPLE_ADDRESSES
void pbpal_multiple_addresses_reset_counters(struct pubnub_multi_addresses* spare_addresses)
{
    spare_addresses->n_ipv4     = 0;
    spare_addresses->ipv4_index = 0;
#if PUBNUB_USE_IPV6
    spare_addresses->n_ipv6     = 0;
    spare_addresses->ipv6_index = 0;
#endif
}


static enum pbpal_resolv_n_connect_result
try_TCP_connect_spare_address(pb_socket_t*                   skt,
                              struct pubnub_multi_addresses* spare_addresses,
                              struct pubnub_options*         options,
                              struct pubnub_flags*           flags,
                              const uint16_t                 port)
{
    enum pbpal_resolv_n_connect_result rslt = pbpal_resolv_resource_failure;
    time_t                             tt   = time(NULL);

    if (spare_addresses->ipv4_index < spare_addresses->n_ipv4
#if PUBNUB_USE_IPV6
        && !options->ipv6_connectivity
#endif /* PUBNUB_USE_IPV6 */
        ) {
        PUBNUB_LOG_TRACE(
            "spare_addresses->ipv4_index = %d, spare_addresses->n_ipv4 = %d.\n",
            spare_addresses->ipv4_index,
            spare_addresses->n_ipv4);
        /* Need at least a second to live */
        if (spare_addresses->ttl_ipv4[spare_addresses->ipv4_index] - 2
            > tt - spare_addresses->time_of_the_last_dns_query) {
            struct sockaddr_in dest = { 0 };
            PUBNUB_LOG_TRACE(
                "spare_addresses->ttl_ipv4[spare_addresses->ipv4_index]=%ld > "
                "(time() - spare_addresses->time_of_the_last_dns_query)=%ld\n",
                (long)(spare_addresses->ttl_ipv4[spare_addresses->ipv4_index]),
                (long)(tt - spare_addresses->time_of_the_last_dns_query));
            memcpy(&(dest.sin_addr.s_addr),
                   spare_addresses->ipv4_addresses[spare_addresses->ipv4_index].ipv4,
                   sizeof dest.sin_addr.s_addr);
            dest.sin_family = AF_INET;
            rslt = connect_TCP_socket(skt, options, (struct sockaddr*)&dest, port);
            if (pbpal_connect_failed == rslt) {
                pbpal_report_error_from_environment(NULL, __FILE__, __LINE__);
            }
        }
        else {
            uint8_t* ipv4 =
                spare_addresses->ipv4_addresses[spare_addresses->ipv4_index].ipv4;
            PUBNUB_LOG_TRACE("Spare IPv4: %d.%d.%d.%d address expired.\n",
                             ipv4[0],
                             ipv4[1],
                             ipv4[2],
                             ipv4[3]);
            rslt = pbpal_connect_failed;
        }
        if (pbpal_connect_failed == rslt) {
            flags->retry_after_close =
                (++spare_addresses->ipv4_index < spare_addresses->n_ipv4);
            if_no_retry_close_socket(skt, flags);
#if PUBNUB_USE_SSL
            flags->trySSL = options->useSSL;
#endif
        }
    }
#if PUBNUB_USE_IPV6
    else if (spare_addresses->ipv6_index < spare_addresses->n_ipv6) {
        PUBNUB_LOG_TRACE(
            "spare_addresses->ipv6_index = %d, spare_addresses->n_ipv6 = %d.\n",
            spare_addresses->ipv6_index,
            spare_addresses->n_ipv6);
        /* Need at least a second to live */
        if (spare_addresses->ttl_ipv6[spare_addresses->ipv6_index] - 2
            > tt - spare_addresses->time_of_the_last_dns_query) {
            struct sockaddr_in6 dest = { 0 };
            PUBNUB_LOG_TRACE(
                "spare_addresses->ttl_ipv6[spare_addresses->ipv6_index]=%ld > "
                "(time() - spare_addresses->time_of_the_last_dns_query)=%ld\n",
                (long)(spare_addresses->ttl_ipv6[spare_addresses->ipv6_index]),
                (long)(tt - spare_addresses->time_of_the_last_dns_query));
            memcpy(dest.sin6_addr.s6_addr,
                   spare_addresses->ipv6_addresses[spare_addresses->ipv6_index].ipv6,
                   sizeof dest.sin6_addr.s6_addr);
            dest.sin6_family = AF_INET6;
            rslt = connect_TCP_socket(skt, options, (struct sockaddr*)&dest, port);
            if (pbpal_connect_failed == rslt) {
                pbpal_report_error_from_environment(NULL, __FILE__, __LINE__);
            }
        }
        else {
            uint8_t* ipv6 =
                spare_addresses->ipv6_addresses[spare_addresses->ipv6_index].ipv6;
            PUBNUB_LOG_TRACE(
                "Spare IPv6: %X:%X:%X:%X:%X:%X:%X:%X address expired.\n",
                ipv6[0] * 256 + ipv6[1],
                ipv6[2] * 256 + ipv6[3],
                ipv6[4] * 256 + ipv6[5],
                ipv6[6] * 256 + ipv6[7],
                ipv6[8] * 256 + ipv6[9],
                ipv6[10] * 256 + ipv6[11],
                ipv6[12] * 256 + ipv6[13],
                ipv6[14] * 256 + ipv6[15]);
            rslt = pbpal_connect_failed;
        }
        if (pbpal_connect_failed == rslt) {
            flags->retry_after_close =
                (++spare_addresses->ipv6_index < spare_addresses->n_ipv6);
            if_no_retry_close_socket(skt, flags);
#if PUBNUB_USE_SSL
            flags->trySSL = options->useSSL;
#endif
        }
    }
#endif /* PUBNUB_USE_IPV6 */
    else {
        pbpal_multiple_addresses_reset_counters(spare_addresses);
    }
    if ((pbpal_connect_failed == rslt) && !flags->retry_after_close) {
        rslt = pbpal_resolv_resource_failure;
        pbpal_multiple_addresses_reset_counters(spare_addresses);
    }

    return rslt;
}
#endif /* PUBNUB_USE_MULTIPLE_ADDRESSES */
#endif /* PUBNUB_CALLBACK_API */


enum pbpal_resolv_n_connect_result pbpal_resolv_and_connect(pubnub_t* pb)
{
    int         error;
    uint16_t    port = HTTP_PORT;
    char const* origin;

#ifdef PUBNUB_NTF_RUNTIME_SELECTION
    if (PNA_CALLBACK == pb->api_policy) { // if policy 
#endif
#ifdef PUBNUB_CALLBACK_API
    sockaddr_inX_t dest = { 0 };
#if PUBNUB_USE_IPV6
    const bool has_ipv6_proxy = 0 != pb->proxy_ipv6_address.ipv6[0]
        || 0 != pb->proxy_ipv6_address.ipv6[1];
#endif /* PUBNUB_USE_IPV6 */

    prepare_port_and_hostname(pb, &port, &origin);
#if PUBNUB_PROXY_API
    if (0 != pb->proxy_ipv4_address.ipv4[0]
#if PUBNUB_USE_IPV6
        && (!pb->options.ipv6_connectivity || !has_ipv6_proxy)
#endif /* PUBNUB_USE_IPV6 */
        ) {
        struct sockaddr_in dest = { 0 };
        PUBNUB_LOG_TRACE("(0 != pb->proxy_ipv4_address.ipv4[0]) - ");
        memcpy(&(dest.sin_addr.s_addr),
               pb->proxy_ipv4_address.ipv4,
               sizeof dest.sin_addr.s_addr);
        dest.sin_family = AF_INET;
        return connect_TCP_socket(
            &pb->pal.socket, &pb->options, (struct sockaddr*)&dest, port);
    }
#if PUBNUB_USE_IPV6
    if (has_ipv6_proxy) {
        struct sockaddr_in6 dest = { 0 };
        PUBNUB_LOG_TRACE("(0 != pb->proxy_ipv6_address.ipv6[0]) ||"
                         " (0 != pb->proxy_ipv6_address.ipv6[1]) - ");
        memcpy(dest.sin6_addr.s6_addr,
               pb->proxy_ipv6_address.ipv6,
               sizeof dest.sin6_addr.s6_addr);
        dest.sin6_family = AF_INET6;
        return connect_TCP_socket(
            &pb->pal.socket, &pb->options, (struct sockaddr*)&dest, port);
    }
#endif /* PUBNUB_USE_IPV6 */
#endif /* PUBNUB_PROXY_API */
#if PUBNUB_USE_MULTIPLE_ADDRESSES
    {
        enum pbpal_resolv_n_connect_result rslt;
        rslt = try_TCP_connect_spare_address(
            &pb->pal.socket, &pb->spare_addresses, &pb->options, &pb->flags, port);
        if (rslt != pbpal_resolv_resource_failure) {
            return rslt;
        }
    }
#endif
#if PUBNUB_CHANGE_DNS_SERVERS
    get_dns_ip(&pb->dns_check, (struct sockaddr*)&dest);
#else
    get_dns_ip((struct sockaddr*)&dest);
#endif
    if (SOCKET_INVALID == pb->pal.socket) {
        pb->pal.socket =
            socket(((struct sockaddr*)&dest)->sa_family, SOCK_DGRAM, IPPROTO_UDP);
    }
    if (SOCKET_INVALID == pb->pal.socket) {
        return pbpal_resolv_resource_failure;
    }
    pb->options.use_blocking_io = false;
    pbpal_set_blocking_io(pb);
    error =
        send_dns_query(pb->pal.socket, (struct sockaddr*)&dest, origin, QUERY_TYPE);
    if (error < 0) {
#if PUBNUB_CHANGE_DNS_SERVERS
        check_dns_server_error(&pb->dns_check, &pb->flags);
        if_no_retry_close_socket(&pb->pal.socket, &pb->flags);
#endif
        return pbpal_resolv_failed_send;
    }
    if (error > 0) {
        return pbpal_resolv_send_wouldblock;
    }
    pb->flags.sent_queries++;
    
    return pbpal_resolv_sent;

#endif /* PUBNUB_CALLBACK_API */
#ifdef PUBNUB_NTF_RUNTIME_SELECTION
    } else { // if policy
#endif
#if !defined PUBNUB_CALLBACK_API || defined PUBNUB_NTF_RUNTIME_SELECTION
    char             port_string[20];
    struct addrinfo* result;
    struct addrinfo* it;
    struct addrinfo  hint;

    hint.ai_socktype = SOCK_STREAM;
    hint.ai_family   = AF_UNSPEC;
    hint.ai_protocol = hint.ai_flags = hint.ai_addrlen = 0;
    hint.ai_addr                                       = NULL;
    hint.ai_canonname                                  = NULL;
    hint.ai_next                                       = NULL;

    prepare_port_and_hostname(pb, &port, &origin);
    snprintf(port_string, sizeof port_string, "%hu", port);
    error = getaddrinfo(origin, port_string, &hint, &result);
    if (error != 0) {
        return pbpal_resolv_failed_processing;
    }
#if PUBNUB_USE_IPV6
    for (int pass = 0; pass < 2; ++pass) {
        bool prioritize_ipv6 = pass == 0 && pb->options.ipv6_connectivity;
#endif /* PUBNUB_USE_IPV6 */

        for (it = result; it != NULL; it = it->ai_next) {
#if PUBNUB_USE_IPV6
            if (prioritize_ipv6 && it->ai_family != AF_INET6) { continue; }
            if (!prioritize_ipv6 && it->ai_family != AF_INET) { continue; }
#endif /* PUBNUB_USE_IPV6 */

            pb->pal.socket =
                socket(it->ai_family, it->ai_socktype, it->ai_protocol);
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
                    PUBNUB_LOG_WARNING("socket connect() failed, will try "
                                       "another IP address, if available\n");
                    socket_close(pb->pal.socket);
                    pb->pal.socket = SOCKET_INVALID;
                    continue;
                }
            }
            break;
        }
#if PUBNUB_USE_IPV6
    }
#endif /* PUBNUB_USE_IPV6 */
    freeaddrinfo(result);

    if (NULL == it) {
        return pbpal_connect_failed;
    }

    socket_set_rcv_timeout(pb->pal.socket, pb->transaction_timeout_ms);
    socket_disable_SIGPIPE(pb->pal.socket);

    return error ? pbpal_connect_wouldblock : pbpal_connect_success;
#endif /* !defined PUBNUB_CALLBACK_API || defined PUBNUB_NTF_RUNTIME_SELECTION */
#ifdef PUBNUB_NTF_RUNTIME_SELECTION
    } // if policy 
#endif
}

#if PUBNUB_USE_MULTIPLE_ADDRESSES
#define PBDNS_OPTIONAL_PARAMS_PB , &pb->spare_addresses, &pb->options
#else
#define PBDNS_OPTIONAL_PARAMS_PB
#endif

enum pbpal_resolv_n_connect_result pbpal_check_resolv_and_connect(pubnub_t* pb)
{
#ifdef PUBNUB_NTF_RUNTIME_SELECTION
    if (PNA_CALLBACK == pb->api_policy) { // if policy 
#endif
#ifdef PUBNUB_CALLBACK_API

    sockaddr_inX_t                     dns_server = { 0 };
    sockaddr_inX_t                     dest       = { 0 };
    uint16_t                           port       = HTTP_PORT;
    enum pbpal_resolv_n_connect_result rslt;

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
#if PUBNUB_CHANGE_DNS_SERVERS
    get_dns_ip(&pb->dns_check, (struct sockaddr*)&dns_server);
#else
    get_dns_ip((struct sockaddr*)&dns_server);
#endif
    switch (read_dns_response(pb->pal.socket,
                              (struct sockaddr*)&dns_server,
                              (struct sockaddr*)&dest PBDNS_OPTIONAL_PARAMS_PB)) {
    case -1:
#if PUBNUB_CHANGE_DNS_SERVERS
        check_dns_server_error(&pb->dns_check, &pb->flags);
#endif
        return pbpal_resolv_failed_rcv;
    case +1:
        return pbpal_resolv_rcv_wouldblock;
    case 0:
#if PUBNUB_CHANGE_DNS_SERVERS
        pb->flags.rotations_count = 0;
#endif /* PUBNUB_CHANGE_DNS_SERVERS */
        break;
    }
    socket_close(pb->pal.socket);

    rslt = connect_TCP_socket(
        &pb->pal.socket, &pb->options, (struct sockaddr*)&dest, port);
#if PUBNUB_USE_MULTIPLE_ADDRESSES
    if (pbpal_connect_failed == rslt) {
        if (AF_INET == ((struct sockaddr*)&dest)->sa_family) {
            pb->flags.retry_after_close =
                (++pb->spare_addresses.ipv4_index < pb->spare_addresses.n_ipv4);
        }
#if PUBNUB_USE_IPV6
        else if (AF_INET6 == ((struct sockaddr*)&dest)->sa_family) {
            pb->flags.retry_after_close =
                (++pb->spare_addresses.ipv6_index < pb->spare_addresses.n_ipv6);
        }
#endif
#if PUBNUB_USE_SSL
        pb->flags.trySSL = pb->options.useSSL;
#endif
    }
#endif /* PUBNUB_USE_MULTIPLE_ADDRESSES */
    return rslt;
#endif /* PUBNUB_CALLBACK_API */
#ifdef PUBNUB_NTF_RUNTIME_SELECTION
    } else { // if policy
#endif
#if !defined PUBNUB_CALLBACK_API || defined PUBNUB_NTF_RUNTIME_SELECTION

    PUBNUB_UNUSED(pb);

    /* Under regular BSD-ish sockets, this function should not be
       called unless using async DNS, so this is an error */
    return pbpal_connect_failed;

#endif /* !defined PUBNUB_CALLBACK_API || defined PUBNUB_NTF_RUNTIME_SELECTION */
#ifdef PUBNUB_NTF_RUNTIME_SELECTION
    } // if policy
#endif
}


enum pbpal_resolv_n_connect_result pbpal_check_connect(pubnub_t* pb)
{
    fd_set         write_set;
    int            rslt;
    struct timeval timev           = { 0, 300000 };
    int            error_code      = 0;
    size_t         error_code_size = sizeof(error_code);

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT(pb->state == PBS_WAIT_CONNECT);

#if defined(_WIN32)
    rslt = getsockopt(
        pb->pal.socket, SOL_SOCKET, SO_ERROR, (char*)&error_code, (int*)&error_code_size);
    if (rslt == SOCKET_ERROR) {
        PUBNUB_LOG_ERROR(
            "Error: pbpal_check_connect(pb=%p)---> getsockopt() == SOCKET_ERROR\n"
            "       WSAGetLastError()=%d\n",
            pb,
            WSAGetLastError());
        return pbpal_connect_failed;
    }
#else
    rslt = getsockopt(
        pb->pal.socket, SOL_SOCKET, SO_ERROR, &error_code, (socklen_t*)&error_code_size);
    if (rslt < 0) {
        PUBNUB_LOG_ERROR(
            "Error: pbpal_check_connect(pb=%p)---> getsockopt()=%d < 0\n", pb, rslt);
        return pbpal_connect_failed;
    }
#endif /* defined(_WIN32) */
    if (error_code != 0) {
        PUBNUB_LOG_ERROR(
            "Error: pbpal_check_connect(pb=%p)--> getsockopt(&error_code)--> "
            "error_code=%d\n",
            pb,
            error_code);
#if PUBNUB_USE_MULTIPLE_ADDRESSES
        PUBNUB_LOG_ERROR(
            "       time_since_the_last_dns_query = %ld seconds\n",
            (long)(time(NULL) - pb->spare_addresses.time_of_the_last_dns_query));
        pb->flags.retry_after_close =
            (++pb->spare_addresses.ipv4_index < pb->spare_addresses.n_ipv4);
#if PUBNUB_USE_IPV6
        if (!pb->flags.retry_after_close) {
            pb->flags.retry_after_close =
                (++pb->spare_addresses.ipv6_index < pb->spare_addresses.n_ipv6);
        }
#endif
#if PUBNUB_USE_SSL
        pb->flags.trySSL = pb->options.useSSL;
#endif
#endif /* PUBNUB_USE_MULTIPLE_ADDRESSES */
        return pbpal_connect_failed;
    }

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
