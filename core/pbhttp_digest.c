/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if PUBNUB_PROXY_API

#include "pubnub_internal.h"

#include "core/pubnub_proxy_core.h"
#include "core/pbhttp_digest.h"

#include "lib/md5/pbmd5.h"
#include "core/pbmd5_to_str.h"
#include "core/pubnub_assert.h"
#include "core/pubnub_log.h"
#include "core/pubnub_generate_uuid.h"

#include <string.h>


void pbhttp_digest_init(struct pbhttp_digest_context* ctx)
{
    PUBNUB_ASSERT_OPT(ctx != NULL);

    ctx->nonce[0] = ctx->opaque[0] = '\0';

    ctx->algorithm = pbhtdigalUnknown;

    ctx->qop = pbhtdigqopNone;

    ctx->client_nonce[0] = '\0';
    ctx->nc              = 1;
}


char const* pubnub_find_str_in_chamebl(pubnub_chamebl_t chamebl, char const* s)
{
    const size_t slen = strlen(s);

    if (slen > 0) {
        size_t i;
        for (i = 0; slen + i < chamebl.size; ++i) {
            if (0 == memcmp(chamebl.ptr + i, s, slen)) {
                return chamebl.ptr + i;
            }
        }
    }

    return NULL;
}


enum pbhttp_digest_parse_header_rslt
pbhttp_digest_parse_header(struct pbhttp_digest_context* ctx,
                           char const*                   header,
                           char*                         realm)
{
    char        empty_str[] = { '\0' };
    char const* s           = header;
    bool        realm_found = false;
    bool        equal_consecutive_realms;

    do {
        pubnub_chamebl_t key   = { empty_str, 0 };
        pubnub_chamebl_t value = { empty_str, 0 };

        s = pbproxy_get_next_key_value(s, &key, &value);
        if (LIT_STR_EQ("realm", key.ptr)) {
            if (pbproxy_check_realm(&value) != 0) {
                return pbhtdig_ParameterError;
            }
            realm_found = true;
            equal_consecutive_realms =
                (strncmp(realm, value.ptr + 1, value.size - 2) == 0)
                && (strlen(realm) == (value.size - 2));
            if (!equal_consecutive_realms) {
                memcpy(realm, value.ptr + 1, value.size - 2);
                realm[value.size - 2] = '\0';
            }
        }
        else if (LIT_STR_EQ("nonce", key.ptr)) {
            const size_t maxnonce = sizeof ctx->nonce / sizeof ctx->nonce[0] - 1;
            size_t       actual_size = value.size - 2;
            if ('"' != *value.ptr) {
                PUBNUB_LOG_ERROR("Received 'nonce' is not quoted\n");
                return pbhtdig_ParameterError;
            }
            if (value.size <= 2) {
                PUBNUB_LOG_ERROR("Received 'nonce' is too short (length %zu)\n",
                                 value.size);
                return pbhtdig_ParameterError;
            }
            if (actual_size > maxnonce) {
                PUBNUB_LOG_ERROR(
                    "Received `nonce` too long: %zu, maximum %zu\n",
                    value.size,
                    maxnonce);
                return pbhtdig_ParameterError;
            }
            memcpy(ctx->nonce, value.ptr + 1, actual_size);
            ctx->nonce[actual_size] = '\0';
            ctx->nc                 = 1;
        }
        else if (LIT_STR_EQ("opaque", key.ptr)) {
            const size_t maxopaque = sizeof ctx->opaque / sizeof ctx->opaque[0] - 1;
            if (value.size > maxopaque) {
                PUBNUB_LOG_ERROR(
                    "Received `opaque` too long: %zu, maximum possible %zu\n",
                    value.size,
                    maxopaque);
                return pbhtdig_ParameterError;
            }
            memcpy(ctx->opaque, value.ptr, value.size);
            ctx->opaque[value.size] = '\0';
        }
        else if (LIT_STR_EQ("algorithm", key.ptr)) {
            if (NULL != pubnub_find_str_in_chamebl(value, "MD5-sess")) {
                ctx->algorithm = pbhtdigalMD5_sess;
            }
            else if (NULL != pubnub_find_str_in_chamebl(value, "MD5")) {
                ctx->algorithm = pbhtdigalMD5;
            }
            else {
                PUBNUB_LOG_WARNING("Unsupported HTTP digest algorithm: %.*s\n",
                                   (int)value.size,
                                   value.ptr);
            }
        }
        else if (LIT_STR_EQ("qop", key.ptr)) {
            if (NULL != pubnub_find_str_in_chamebl(value, "auth-int")) {
                ctx->qop = pbgtdigqop_auth_int;
            }
            else if (NULL != pubnub_find_str_in_chamebl(value, "auth")) {
                ctx->qop = pbgtdigqop_auth;
            }
        }
        else if (LIT_STR_EQ("stale", key.ptr)) {
            if (NULL != pubnub_find_str_in_chamebl(value, "true")) {
                ctx->nc = 1;
            }
        }
        else {
            /* We ignore `domain`, it has no meaning for HTTP proxies.

               We ignore `charset` as it is only available in latest
               specs (RFC 7616) and actually has only UTF-8 as the
               allowed value.

               We ignore `userhash` as it is only available in latest
               specs (RFC 7616) and we don't support it yet.
            */
            PUBNUB_LOG_INFO("Unsupported HTTP digest auth field: %.*s\n",
                            (int)key.size,
                            key.ptr);
        }
    } while (s != NULL);
    if (realm_found) {
        return equal_consecutive_realms ? pbhtdig_EqualConsecutiveRealms
                                        : pbhtdig_DifferentConsecutiveRealms;
    }

    return pbhtdig_RealmNotFound;
}


char const* pbhttp_digest_qop2str(enum pbhttp_digest_qop e)
{
    switch (e) {
    case pbhtdigqopNone:
        return "";
    case pbgtdigqop_auth:
        return "auth";
    case pbgtdigqop_auth_int:
        return "auth-int";
    default:
        return "(enum pbhttp_digest_qop)?!";
    }
}


char const* pbhttp_digest_algorithm2str(enum pbhttp_digest_algorithm e)
{
    switch (e) {
    case pbhtdigalUnknown:
        return "";
    case pbhtdigalMD5:
        return "MD5";
    case pbhtdigalMD5_sess:
        return "MD5-sess";
    default:
        return "(enum pbhttp_digest_algorithm)?!";
    }
}


#define md5_final_to_str(x, dstr)                                              \
    do {                                                                       \
        uint8_t M_md5_store_[16];                                              \
        PUBNUB_ASSERT_OPT(sizeof(dstr) == 33);                                 \
        pbmd5_final((x), M_md5_store_);                                        \
        pbmd5_to_str(M_md5_store_, (dstr));                                    \
    } while (0)


#define MD5_OF_EMPTY_MESSAGE "d41d8cd98f00b204e9800998ecf8427e"


int pbhttp_digest_prep_header_to_send(struct pbhttp_digest_context* ctx,
                                      char const*                   username,
                                      char const*                   password,
                                      char const*                   uri,
                                      char const*                   realm,
                                      pubnub_chamebl_t*             buf)
{
    char      ha1[33];
    char      ha2[33];
    char      response[33];
    PBMD5_CTX md5;

    PUBNUB_ASSERT_OPT(ctx != NULL);
    PUBNUB_ASSERT_OPT(username != NULL);
    PUBNUB_ASSERT_OPT(password != NULL);
    PUBNUB_ASSERT_OPT(uri != NULL);
    PUBNUB_ASSERT_OPT(realm != NULL);

    if ((ctx->qop != pbhtdigqopNone) && ('\0' == ctx->nonce[0])) {
        PUBNUB_LOG_ERROR(
            "HTTP digest `qop` defined, but haven't received `nonce`\n");
        return -1;
    }

    if ('\0' == ctx->client_nonce[0]) {
        struct Pubnub_UUID uuid;
        pubnub_generate_uuid_v4_random(&uuid);
        pbmd5_to_str(uuid.uuid, ctx->client_nonce);
    }

    pbmd5_init(&md5);
    pbmd5_update(&md5, username, strlen(username));
    pbmd5_update(&md5, ":", 1);
    pbmd5_update(&md5, realm, strlen(realm));
    pbmd5_update(&md5, ":", 1);
    pbmd5_update(&md5, password, strlen(password));
    md5_final_to_str(&md5, ha1);

    if (pbhtdigalMD5_sess == ctx->algorithm) {
        pbmd5_init(&md5);
        pbmd5_update(&md5, ha1, 32);
        pbmd5_update(&md5, ":", 1);
        pbmd5_update(&md5, ctx->nonce, strlen(ctx->nonce));
        pbmd5_update(&md5, ":", 1);
        pbmd5_update(&md5, ctx->client_nonce, strlen(ctx->client_nonce));
        md5_final_to_str(&md5, ha1);
    }

    /* We use only GET requests, without body */
    pbmd5_init(&md5);
    pbmd5_update(&md5, "GET:", 4);
    pbmd5_update(&md5, uri, strlen(uri));
    if (pbgtdigqop_auth_int == ctx->qop) {
        pbmd5_update(&md5, ":" MD5_OF_EMPTY_MESSAGE, 33);
    }
    md5_final_to_str(&md5, ha2);

    pbmd5_init(&md5);
    pbmd5_update(&md5, ha1, 32);
    pbmd5_update(&md5, ":", 1);
    pbmd5_update(&md5, ctx->nonce, strlen(ctx->nonce));
    if (pbhtdigqopNone != ctx->qop) {
        char const* qop_str = pbhttp_digest_qop2str(ctx->qop);
        char        nonce_count_str[9];
        snprintf(nonce_count_str, sizeof nonce_count_str, "%08x", ctx->nc);
        pbmd5_update(&md5, ":", 1);
        pbmd5_update(&md5, nonce_count_str, sizeof nonce_count_str - 1);
        pbmd5_update(&md5, ":", 1);
        pbmd5_update(&md5, ctx->client_nonce, strlen(ctx->client_nonce));
        pbmd5_update(&md5, ":", 1);
        pbmd5_update(&md5, qop_str, strlen(qop_str));
    }
    pbmd5_update(&md5, ":", 1);
    pbmd5_update(&md5, ha2, strlen(ha2));
    md5_final_to_str(&md5, response);

    if (pbhtdigqopNone == ctx->qop) {
        buf->size =
            snprintf(buf->ptr,
                     buf->size,
                     "username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", "
                     "response=\"%s\""
                     "%s%s"
                     "%s%s",
                     username,
                     realm,
                     ctx->nonce,
                     uri,
                     response,
                     ctx->opaque[0] == '\0' ? "" : ", opaque=",
                     ctx->opaque,
                     (pbhtdigalUnknown == ctx->algorithm) ? "" : ", algorithm=",
                     pbhttp_digest_algorithm2str(ctx->algorithm));
    }
    else {
        buf->size =
            snprintf(buf->ptr,
                     buf->size,
                     "username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", "
                     "cnonce=\"%s\", nc=\"%08x\", qop=\"%s\", response=\"%s\""
                     "%s%s"
                     "%s%s",
                     username,
                     realm,
                     ctx->nonce,
                     uri,
                     ctx->client_nonce,
                     ctx->nc,
                     pbhttp_digest_qop2str(ctx->qop),
                     response,
                     (ctx->opaque[0] == '\0' ? "" : ", opaque="),
                     ctx->opaque,
                     (pbhtdigalUnknown == ctx->algorithm) ? "" : ", algorithm=",
                     pbhttp_digest_algorithm2str(ctx->algorithm));
        ++ctx->nc;
    }

    return 0;
}

#endif /* PUBNUB_PROXY_API */
