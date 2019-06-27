/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#include "pubnub_proxy.h"
#include "pubnub_assert.h"
#include "pubnub_log.h"
#if defined(PUBNUB_CALLBACK_API)
#include "lib/pubnub_parse_ipv4_addr.h"
#if PUBNUB_USE_IPV6
#include "lib/pubnub_parse_ipv6_addr.h"
#endif
#endif /* defined(PUBNUB_CALLBACK_API) */

#include <string.h>


int pubnub_set_proxy_manual(pubnub_t*              p,
                            enum pubnub_proxy_type protocol,
                            char const*            ip_address_or_url,
                            uint16_t               port)
{
    size_t ip_or_url_len;

    PUBNUB_ASSERT_OPT(p != NULL);
    PUBNUB_ASSERT_OPT(ip_address_or_url != NULL);

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
    if (ip_or_url_len >= sizeof p->proxy_hostname) {
        return -1;
    }
    p->proxy_type = protocol;
    p->proxy_port = port;
#if defined(PUBNUB_CALLBACK_API)
    /* If we haven't got numerical address for proxy we'll have to do DNS resolution(from proxy
       host name) later on, but in order to do that we have to have all proxy addresses(on the
       given context) set to zeros.
     */
    if (0 != pubnub_parse_ipv4_addr(ip_address_or_url, &(p->proxy_ipv4_address))) {
        memset(&(p->proxy_ipv4_address), 0, sizeof p->proxy_ipv4_address);
#if PUBNUB_USE_IPV6
        if (0 != pubnub_parse_ipv6_addr(ip_address_or_url, &(p->proxy_ipv6_address))) {
            memset(&(p->proxy_ipv6_address), 0, sizeof p->proxy_ipv6_address);
        }
#endif
    }
#endif /* defined(PUBNUB_CALLBACK_API) */
    memcpy(p->proxy_hostname, ip_address_or_url, ip_or_url_len + 1);

    return 0;
}


void pubnub_set_proxy_none(pubnub_t* p)
{
    PUBNUB_ASSERT_OPT(p != NULL);

#if defined(PUBNUB_CALLBACK_API)
    memset(&(p->proxy_ipv4_address), 0, sizeof p->proxy_ipv4_address);
#if PUBNUB_USE_IPV6
    memset(&(p->proxy_ipv6_address), 0, sizeof p->proxy_ipv6_address);
#endif
#endif /* defined(PUBNUB_CALLBACK_API) */
    p->proxy_type = pbproxyNONE;
    p->proxy_port = 0;
    p->proxy_hostname[0] = '\0';
}
    

enum pubnub_proxy_type pubnub_proxy_protocol_get(pubnub_t* p)
{
    PUBNUB_ASSERT_OPT(p != NULL);

    return p->proxy_type;
}


int pubnub_proxy_get_config(pubnub_t const*         pb,
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

    *protocol = pb->proxy_type;
    *port     = pb->proxy_port;
    hnlen     = strlen(pb->proxy_hostname);
    if (hnlen + 1 < n) {
        return -1;
    }
    memcpy(host, pb->proxy_hostname, hnlen + 1);

    return 0;
}


int pubnub_set_proxy_authentication_username_password(pubnub_t*   p,
                                                      char const* username,
                                                      char const* password)
{
    PUBNUB_ASSERT_OPT(p != NULL);

    p->proxy_auth_username = username;
    p->proxy_auth_password = password;

    return 0;
}
