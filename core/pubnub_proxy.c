/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_proxy.h"

#include "pubnub_assert.h"
#include "pubnub_internal.h"

#include <string.h>


int pubnub_set_proxy_manual(pubnub_t *p, enum pubnub_proxy_type protocol, char const *ip_address_or_url, uint16_t port)
{
    size_t ip_or_url_len;

    PUBNUB_ASSERT_OPT(p != NULL);
    PUBNUB_ASSERT_OPT(ip_address_or_url != NULL);

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
    memcpy(p->proxy_hostname, ip_address_or_url, ip_or_url_len + 1);

    return 0;
}


enum pubnub_proxy_type pubnub_proxy_protocol_get(pubnub_t *p)
{
    PUBNUB_ASSERT_OPT(p != NULL);

    return p->proxy_type;
}


int pubnub_set_proxy_authentication_username_password(pubnub_t *p, char const *username, char const *password)
{
    PUBNUB_ASSERT_OPT(p != NULL);

    p->proxy_auth_username = username;
    p->proxy_auth_password = password;

    return 0;
}


