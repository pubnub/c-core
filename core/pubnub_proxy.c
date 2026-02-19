/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if PUBNUB_PROXY_API

#include "pubnub_internal.h"

#include "pubnub_proxy.h"
#include "pubnub_assert.h"
#ifdef PUBNUB_NTF_RUNTIME_SELECTION
#include "pubnub_ntf_enforcement.h"
#endif
#if PUBNUB_USE_LOGGER
#include "pbcc_logger_manager.h"
#endif // PUBNUB_USE_LOGGER
#ifdef PUBNUB_CALLBACK_API
#include "lib/pubnub_parse_ipv4_addr.h"
#if PUBNUB_USE_IPV6
#include "lib/pubnub_parse_ipv6_addr.h"
#endif
#include "pbpal.h"
#endif /* defined(PUBNUB_CALLBACK_API) */

#include <string.h>


int pubnub_set_proxy_manual(
    pubnub_t*              p,
    enum pubnub_proxy_type protocol,
    char const*            ip_address_or_url,
    uint16_t               port)
{
    size_t ip_or_url_len;

    PUBNUB_ASSERT_OPT(p != NULL);
    PUBNUB_ASSERT_OPT(ip_address_or_url != NULL);

#if PUBNUB_LOG_ENABLED(DEBUG)
    if (pubnub_logger_should_log(p, PUBNUB_LOG_LEVEL_DEBUG)) {
        pubnub_log_value_t data = pubnub_log_value_map_init();
        PUBNUB_LOG_MAP_SET_NUMBER(&data, protocol)
        PUBNUB_LOG_MAP_SET_STRING(&data, ip_address_or_url)
        PUBNUB_LOG_MAP_SET_NUMBER(&data, port)
        pubnub_log_object(
            p,
            PUBNUB_LOG_LEVEL_DEBUG,
            PUBNUB_LOG_LOCATION,
            &data,
            "Ser proxy manually with parameters:");
    }
#endif

    if (0 == strcmp("localhost", ip_address_or_url)) {
        ip_address_or_url = "127.0.0.1";
    }
    switch (protocol) {
    case pbproxyHTTP_GET:
    case pbproxyHTTP_CONNECT:
        break;
    default:
        /* other proxy protocols not yet supported */
        return -1;
    }

    ip_or_url_len = strlen(ip_address_or_url);
    if (ip_or_url_len >= sizeof p->proxy_hostname) { return -1; }
    pubnub_mutex_lock(p->monitor);
    p->proxy_type = protocol;
    p->proxy_port = port;
#if defined(PUBNUB_CALLBACK_API)
#if defined(PUBNUB_NTF_RUNTIME_SELECTION)
    if (PNA_CALLBACK == p->api_policy) {
#endif
        /* If we haven't got numerical address for proxy we'll have to do DNS
           resolution(from proxy host name) later on, but in order to do that we
           have to have all proxy addresses(on the given context) set to zeros.
         */
        if (0 != pubnub_parse_ipv4_addr(
                     ip_address_or_url, &(p->proxy_ipv4_address))) {
            memset(&(p->proxy_ipv4_address), 0, sizeof p->proxy_ipv4_address);
#if PUBNUB_USE_IPV6
            if (0 != pubnub_parse_ipv6_addr(
                         p, ip_address_or_url, &(p->proxy_ipv6_address))) {
                memset(
                    &(p->proxy_ipv6_address), 0, sizeof p->proxy_ipv6_address);
            }
#endif
        }
#if PUBNUB_USE_MULTIPLE_ADDRESSES
        pbpal_multiple_addresses_reset_counters(&p->spare_addresses);
#endif
#if defined(PUBNUB_NTF_RUNTIME_SELECTION)
    } /* if (PNA_CALLBACK == p->api_policy) */
#endif
#endif /* defined(PUBNUB_CALLBACK_API) */
    memcpy(p->proxy_hostname, ip_address_or_url, ip_or_url_len + 1);
    pubnub_mutex_unlock(p->monitor);

    return 0;
}


void pubnub_set_proxy_none(pubnub_t* p)
{
    PUBNUB_ASSERT_OPT(p != NULL);

    PUBNUB_LOG_DEBUG(p, "Disable proxy.");

    pubnub_mutex_lock(p->monitor);
#if defined(PUBNUB_CALLBACK_API)
#if defined(PUBNUB_NTF_RUNTIME_SELECTION)
    if (PNA_CALLBACK == p->api_policy) {
#endif
        memset(&(p->proxy_ipv4_address), 0, sizeof p->proxy_ipv4_address);
#if PUBNUB_USE_IPV6
        memset(&(p->proxy_ipv6_address), 0, sizeof p->proxy_ipv6_address);
#endif
#if PUBNUB_USE_MULTIPLE_ADDRESSES
        pbpal_multiple_addresses_reset_counters(&p->spare_addresses);
#endif
#if defined(PUBNUB_NTF_RUNTIME_SELECTION)
    } /* if (PNA_CALLBACK == p->api_policy) */
#endif
#endif /* defined(PUBNUB_CALLBACK_API) */
    p->proxy_type        = pbproxyNONE;
    p->proxy_port        = 0;
    p->proxy_hostname[0] = '\0';
    pubnub_mutex_unlock(p->monitor);
}


enum pubnub_proxy_type pubnub_proxy_protocol_get(pubnub_t* p)
{
    enum pubnub_proxy_type proxy_type;

    PUBNUB_ASSERT_OPT(p != NULL);

    pubnub_mutex_lock(p->monitor);
    proxy_type = p->proxy_type;
    pubnub_mutex_unlock(p->monitor);

    return proxy_type;
}


int pubnub_proxy_get_config(
    pubnub_t*               pb,
    enum pubnub_proxy_type* protocol,
    uint16_t*               port,
    char*                   host,
    unsigned                n)
{
    size_t hnlen;

    PUBNUB_ASSERT_OPT(pb != NULL);
    PUBNUB_ASSERT_OPT(protocol != NULL);
    PUBNUB_ASSERT_OPT(port != NULL);
    PUBNUB_ASSERT_OPT(host != NULL);

    pubnub_mutex_lock(pb->monitor);
    *protocol = pb->proxy_type;
    *port     = pb->proxy_port;
    hnlen     = strlen(pb->proxy_hostname);
    if (hnlen + 1 < n) {
        pubnub_mutex_unlock(pb->monitor);
        return -1;
    }
    memcpy(host, pb->proxy_hostname, hnlen + 1);
    pubnub_mutex_unlock(pb->monitor);

    return 0;
}


int pubnub_set_proxy_authentication_username_password(
    pubnub_t*   p,
    char const* username,
    char const* password)
{
    PUBNUB_ASSERT_OPT(p != NULL);

#if PUBNUB_LOG_ENABLED(DEBUG)
    if (pubnub_logger_should_log(p, PUBNUB_LOG_LEVEL_DEBUG)) {
        pubnub_log_value_t data = pubnub_log_value_map_init();
        PUBNUB_LOG_MAP_SET_STRING(&data, username)
        PUBNUB_LOG_MAP_SET_STRING(&data, password)
        pubnub_log_object(
            p,
            PUBNUB_LOG_LEVEL_DEBUG,
            PUBNUB_LOG_LOCATION,
            &data,
            "Set proxy authentication:");
    }
#endif

    pubnub_mutex_lock(p->monitor);
    p->proxy_auth_username = username;
    p->proxy_auth_password = password;
    pubnub_mutex_unlock(p->monitor);

    return 0;
}

#endif /* PUBNUB_PROXY_API */
