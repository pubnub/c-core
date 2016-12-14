/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_proxy_core.h"

#include "pubnub_assert.h"
#include "pubnub_log.h"

#include "pbbase64.h"
#include "pbntlm_core.h"

#include <string.h>



void pbproxy_handle_http_header(pubnub_t *p, char const* header)
{
    char scheme_basic[] = "Basic";
    char scheme_NTLM[] = "NTLM";
    char proxy_auth[] = "Proxy-Authenticate: ";
    char const* contents;

    PUBNUB_ASSERT_OPT(p != NULL);
    PUBNUB_ASSERT_OPT(header != NULL);

    if (strncmp(p->core.http_buf, proxy_auth, sizeof proxy_auth - 1) != 0) {
        return;
    }
    contents = p->core.http_buf + sizeof proxy_auth - 1;

    PUBNUB_LOG_TRACE("pbproxy_handle_http_header(header='%s', contents='%s')\n", header, contents);

    if (0 == strncmp(contents, scheme_basic, sizeof scheme_basic -1)) {
        /* We ignore the "realm" for now */
        PUBNUB_LOG_TRACE("pbproxy_handle_http_header() Basic authentication\n");
        p->proxy_auth_scheme = pbhtauBasic;
        p->proxy_authorization_sent = false;
    }
    else if (0 == strncmp(contents, scheme_NTLM, sizeof scheme_NTLM -1)) {
        if (pbhtauNone == p->proxy_auth_scheme) {
            pbntlm_core_init(p);
            p->proxy_auth_scheme = pbhtauNTLM;
            p->proxy_authorization_sent = false;
        }
        else {
            pbntlm_core_handle(p, contents + sizeof scheme_NTLM);
        }
    }
    else {
        PUBNUB_LOG_ERROR("Proxy Authentication '%s' not supported\n", contents);
        p->proxy_auth_scheme = pbhtauNone;
    }
}


static char const* pbproxy_pal_get_username(void)
{
    return "";
}

static char const* pbproxy_pal_get_password(void)
{
    return "";
}


static char const* figure_out_username(pubnub_t *p)
{
    return (NULL == p->proxy_auth_username) ? pbproxy_pal_get_username() : p->proxy_auth_username;
}


static char const* figure_out_password(pubnub_t *p)
{
    return (NULL == p->proxy_auth_password) ? pbproxy_pal_get_password() : p->proxy_auth_password;
}


int pbproxy_http_header_to_send(pubnub_t *p, char* header, size_t n)
{
    PUBNUB_ASSERT_OPT(p != NULL);
    PUBNUB_ASSERT_OPT(header != NULL);

    switch (p->proxy_auth_scheme) {
    case pbhtauBasic:
    {
        int i;
        char prefix[] = "Proxy-Authorization: Basic ";
        char response[128];
        pubnub_bymebl_t data = { (uint8_t*)response, 0 };

        PUBNUB_ASSERT_OPT(n > sizeof prefix);
        memcpy(header, prefix, sizeof prefix);
        n -= sizeof prefix;
        data.size = snprintf(response, sizeof response, "%s:%s", figure_out_username(p), figure_out_password(p));

        PUBNUB_LOG_TRACE("pbproxy_http_header_to_send(): Basic header (before Base64): '%s'\n", response);

        i = pbbase64_encode_std(data, header + sizeof prefix - 1, &n);
        if (0 == i) {
            PUBNUB_LOG_TRACE("pbproxy_http_header_to_send(): Basic header (after Base64): '%s'\n", header);
            p->proxy_authorization_sent = true;
        }
        else {
            PUBNUB_LOG_ERROR("pbproxy_http_header_to_send(): Basic Failed Base64 encoding of header\n");
        }
        return i;
    }
    case pbhtauNTLM:
    {
        char prefix[] = "Proxy-Authorization: NTLM ";
        uint8_t response[256];
        pubnub_bymebl_t data = { response, sizeof response };
        int i = pbntlm_core_prep_msg_to_send(p, &data);

        if (i != 0) {
            PUBNUB_LOG_ERROR("pbproxy_http_header_to_send(): NTLM failed preparing message to send'\n");
            return -1;
        }

        PUBNUB_ASSERT_OPT(n > sizeof prefix);
        memcpy(header, prefix, sizeof prefix);
        n -= sizeof prefix;

        i = pbbase64_encode_std(data, header + sizeof prefix - 1, &n);
        if (0 == i) {
            PUBNUB_LOG_TRACE("pbproxy_http_header_to_send(): NTLM header (after Base64): '%s'\n", header);
        }
        else {
            PUBNUB_LOG_ERROR("pbproxy_http_header_to_send(): NTLM Failed Base64 encoding of header\n");
        }

        return i;
    }
    case pbhtauNone:
        return -1;
    default:
        PUBNUB_LOG_ERROR("pbproxy_http_header_to_send(): Proxy auth scheme %d not supported\n", p->proxy_auth_scheme);
        return -1;
    }
}


enum pbproxyFinInstruction pbproxy_handle_finish(pubnub_t *pb)
{
    if (pb->proxy_type == pbproxyHTTP_CONNECT) {
        if (!pb->proxy_tunnel_established) {
            if ((pb->http_code / 100) != 2) {
                return pbproxyFinError;
            }
            else {
                pb->proxy_tunnel_established = true;
                return pbproxyFinRetryConnected;
            }
        }
        else {
            pb->proxy_tunnel_established = false;
        }
    }
    else if (pb->proxy_type == pbproxyHTTP_GET) {
        switch (pb->proxy_auth_scheme) {
        case pbhtauBasic:
            if ((pb->http_code == 407) && !pb->proxy_authorization_sent) {
                return pbproxyFinRetryConnected;
            }
            break;
        case pbhtauNTLM:
            if ((pb->http_code == 407) && !pb->proxy_authorization_sent) {
                switch (pb->ntlm_context.state) {
                case pbntlmIdle: return pbproxyFinRetryReconnect;
                case pbntlmDone: break;
                default: return  pbproxyFinRetryConnected;
                }
            }
            break;
        default:
            break;
        }
    }

    return pbproxyFinGoOn;
}
