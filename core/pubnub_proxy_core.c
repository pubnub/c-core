/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "core/pubnub_proxy_core.h"

#include "core/pubnub_assert.h"
#include "core/pubnub_log.h"

#include "lib/base64/pbbase64.h"
#include "core/pbntlm_core.h"
#include "core/pbhttp_digest.h"

#include <string.h>


/** RFC 7235: The 407 (Proxy Authentication Required) status code is
   similar to 401 (Unauthorized), but it indicates that the client
   needs to authenticate itself in order to use a proxy.  The proxy
   MUST send a Proxy-Authenticate header field containing a challenge
   applicable to that proxy for the target resource.  The client MAY
   repeat the request with a new or replaced Proxy-Authorization
   header field */
#define HTTP_CODE_PROXY_AUTH_REQ 407


void pbproxy_handle_http_header(pubnub_t* p, char const* header)
{
    char        scheme_basic[]  = "Basic";
    char        scheme_digest[] = "Digest";
    char        scheme_NTLM[]   = "NTLM";
    char        proxy_auth[]    = "Proxy-Authenticate: ";
    char const* contents;

    PUBNUB_ASSERT_OPT(p != NULL);
    PUBNUB_ASSERT_OPT(header != NULL);

    switch (header[0]) {
    case ' ':
    case '\t':
        /* Though this is not very nice, we only support multi-line
           headers for Digest proxy. In practice, Basic and NTLM never
           use multi-line headers.
        */
        if (p->proxy_auth_scheme != pbhtauDigest) {
            return;
        }
        pbhttp_digest_parse_header(&p->digest_context, header + 1);
        return;
    default:
        if (strncmp(header, proxy_auth, sizeof proxy_auth - 1) != 0) {
            return;
        }
        break;
    }
    contents = header + sizeof proxy_auth - 1;

    PUBNUB_LOG_TRACE("pbproxy_handle_http_header(header='%s', contents='%s')\n",
                     header,
                     contents);

    if (0 == strncmp(contents, scheme_basic, sizeof scheme_basic - 1)) {
        /* We ignore the "realm" for now */
        PUBNUB_LOG_TRACE("pbproxy_handle_http_header() Basic authentication\n");
        p->proxy_auth_scheme        = pbhtauBasic;
        p->proxy_authorization_sent = false;
    }
    else if (0 == strncmp(contents, scheme_digest, sizeof scheme_digest - 1)) {
        /* We ignore the "realm" for now */
        PUBNUB_LOG_TRACE(
            "pbproxy_handle_http_header() Digest authentication\n");
        p->proxy_auth_scheme = pbhtauDigest;
        pbhttp_digest_init(&p->digest_context);
        pbhttp_digest_parse_header(&p->digest_context,
                                   contents + sizeof scheme_digest);
        p->proxy_authorization_sent = false;
    }
    else if (0 == strncmp(contents, scheme_NTLM, sizeof scheme_NTLM - 1)) {
        if (pbhtauNTLM != p->proxy_auth_scheme) {
            pbntlm_core_init(p);
            p->proxy_auth_scheme        = pbhtauNTLM;
            p->proxy_authorization_sent = false;
        }
        else {
            char const* base64_msg = contents + sizeof scheme_NTLM;
            pbntlm_core_handle(p, base64_msg, strcspn(base64_msg, " \r\n"));
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


static char const* figure_out_username(pubnub_t* p)
{
    return (NULL == p->proxy_auth_username) ? pbproxy_pal_get_username()
                                            : p->proxy_auth_username;
}


static char const* figure_out_password(pubnub_t* p)
{
    return (NULL == p->proxy_auth_password) ? pbproxy_pal_get_password()
                                            : p->proxy_auth_password;
}


int pbproxy_http_header_to_send(pubnub_t* p, char* header, size_t n)
{
    PUBNUB_ASSERT_OPT(p != NULL);
    PUBNUB_ASSERT_OPT(header != NULL);

    switch (p->proxy_auth_scheme) {
    case pbhtauBasic: {
        int             i;
        char            prefix[] = "Proxy-Authorization: Basic ";
        char            response[128];
        pubnub_bymebl_t data = { (uint8_t*)response, 0 };

        PUBNUB_ASSERT_OPT(n > sizeof prefix);
        memcpy(header, prefix, sizeof prefix);
        n -= sizeof prefix;
        data.size = snprintf(response,
                             sizeof response,
                             "%s:%s",
                             figure_out_username(p),
                             figure_out_password(p));

        PUBNUB_LOG_TRACE("pbproxy_http_header_to_send(): Basic header (before "
                         "Base64): '%s'\n",
                         response);

        i = pbbase64_encode_std(data, header + sizeof prefix - 1, &n);
        if (0 == i) {
            PUBNUB_LOG_TRACE("pbproxy_http_header_to_send(): Basic header "
                             "(after Base64): '%s'\n",
                             header);
            p->proxy_authorization_sent = true;
        }
        else {
            PUBNUB_LOG_ERROR("pbproxy_http_header_to_send(): Basic Failed "
                             "Base64 encoding of header\n");
        }
        return i;
    }
    case pbhtauDigest: {
        char             prefix[] = "Proxy-Authorization: Digest ";
        pubnub_chamebl_t data     = { header + sizeof prefix - 1,
                                  n - (sizeof prefix - 1) };

        memcpy(header, prefix, sizeof prefix);

        if (0
            == pbhttp_digest_prep_header_to_send(&p->digest_context,
                                                 figure_out_username(p),
                                                 figure_out_password(p),
                                                 p->proxy_saved_path,
                                                 &data)) {
            PUBNUB_LOG_TRACE(
                "pbproxy_http_header_to_send(): Digest header: '%s'\n", header);
        }
        else {
            PUBNUB_LOG_ERROR("Failed to prepare HTTP digest auth header\n");
            return -1;
        }
        return 0;
    }
    case pbhtauNTLM: {
        char            prefix[] = "Proxy-Authorization: NTLM ";
        uint8_t         response[PUBNUB_NTLM_MAX_TOKEN];
        pubnub_bymebl_t data = { response, sizeof response };
        int             i    = pbntlm_core_prep_msg_to_send(p, &data);

        if (i != 0) {
            PUBNUB_LOG_ERROR("pbproxy_http_header_to_send(): NTLM failed "
                             "preparing message to send'\n");
            return -1;
        }
        if (0 == data.size) {
            return -1;
        }

        PUBNUB_ASSERT_OPT(n > sizeof prefix);
        memcpy(header, prefix, sizeof prefix);
        n -= sizeof prefix;

        i = pbbase64_encode_std(data, header + sizeof prefix - 1, &n);
        if (0 == i) {
            PUBNUB_LOG_TRACE("pbproxy_http_header_to_send(): NTLM header "
                             "(after Base64): '%s'\n",
                             header);
        }
        else {
            PUBNUB_LOG_ERROR("pbproxy_http_header_to_send(): NTLM Failed "
                             "Base64 encoding of header\n");
        }

        return i;
    }
    case pbhtauNone:
        return -1;
    default:
        PUBNUB_LOG_ERROR("pbproxy_http_header_to_send(): Proxy auth scheme %d "
                         "not supported\n",
                         p->proxy_auth_scheme);
        return -1;
    }
}


enum pbproxyFinInstruction pbproxy_handle_finish(pubnub_t* pb)
{
    if (HTTP_CODE_PROXY_AUTH_REQ == pb->http_code) {
        if (pb->proxy_authorization_sent || (pb->proxy_auth_scheme == pbhtauNone)) {
            return pbproxyFinError;
        }
        else {
            return pbproxyFinRetry;
        }
    }
    if (pb->proxy_type == pbproxyHTTP_CONNECT) {
        if (!pb->proxy_tunnel_established && ((pb->http_code / 100) == 2)) {
            pb->proxy_tunnel_established = true;
            return pbproxyFinRetry;
        }
    }

    return pbproxyFinGoOn;
}
