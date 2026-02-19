/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"
#include "pubnub_version.h"
#include "pubnub_assert.h"
#include "pubnub_helper.h"
#if PUBNUB_USE_LOGGER
#include "pbcc_logger_manager.h"
#endif // PUBNUB_USE_LOGGER
#include "pubnub_url_encode.h"
#include "lib/pb_strnlen_s.h"
#include "pbcc_revoke_token_api.h"

#include "pbpal.h"
#include <stdio.h>
#include <stdlib.h>


enum pubnub_res pbcc_revoke_token_prep(
    struct pbcc_context* pb,
    char const*          token,
    enum pubnub_trans    pt)
{
    char const* const uname   = pubnub_uname();
    char const*       user_id = pbcc_user_id_get(pb);
    enum pubnub_res   rslt    = PNR_OK;

    PUBNUB_ASSERT_OPT(user_id != NULL);
    PUBNUB_ASSERT_OPT(token != NULL);

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len          = snprintf(
        pb->http_buf,
        sizeof pb->http_buf,
        "/v3/pam/%s/grant/",
        pb->subscribe_key);

    APPEND_URL_ENCODED_M_TRANS(PBTT_REVOKE_TOKEN, pb, token);
    URL_PARAMS_INIT(qparam, PUBNUB_MAX_URL_PARAMS);
    if (uname) { ADD_URL_PARAM(qparam, pnsdk, uname); }
    if (user_id) { ADD_URL_PARAM(qparam, uuid, user_id); }

#if PUBNUB_CRYPTO_API
    ADD_TS_TO_URL_PARAM();
    SORT_URL_PARAMETERS(qparam);
#endif
    ENCODE_URL_PARAMS_TRANS(pt, pb, qparam);
#if PUBNUB_CRYPTO_API
    if (pb->secret_key != NULL) {
        rslt = pbcc_sign_url(pb, token, pubnubUseDELETE, true);
        if (rslt != PNR_OK) {
#if PUBNUB_LOG_ENABLED(ERROR)
            pbcc_logger_manager_log_error(
                pb->logger_manager,
                PUBNUB_LOG_LEVEL_ERROR,
                PUBNUB_LOG_LOCATION,
                rslt,
                "Revoke token URL signing failed",
                pubnub_res_2_string(rslt));
#endif // PUBNUB_LOG_ENABLED(ERROR)
        }
    }
#endif
    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


pubnub_chamebl_t pbcc_get_revoke_token_response(struct pbcc_context* pb)
{
    pubnub_chamebl_t result;
    char const*      reply    = pb->http_reply;
    int              replylen = pb->http_buf_len;

    if (pb->last_result != PNR_OK) {
#if PUBNUB_LOG_ENABLED(ERROR)
        pbcc_logger_manager_log_error(
            pb->logger_manager,
            PUBNUB_LOG_LEVEL_ERROR,
            PUBNUB_LOG_LOCATION,
            pb->last_result,
            "Unexpected or failed transaction",
            pubnub_res_2_string(pb->last_result));
#endif // PUBNUB_LOG_ENABLED(ERROR)
        result.ptr  = NULL;
        result.size = 0;
        return result;
    }

    result.ptr  = (char*)reply;
    result.size = replylen;
    return result;
}


enum pubnub_res pbcc_parse_revoke_token_response(struct pbcc_context* pb)
{
    char*                                reply    = pb->http_reply;
    int                                  replylen = pb->http_buf_len;
    struct pbjson_elem                   elem;
    struct pbjson_elem                   parsed;
    enum pbjson_object_name_parse_result json_rslt;

    PBCC_LOG_TRACE(
        pb->logger_manager, "Parsing revoke token service response...");

    if ((replylen < 2) || (reply[0] != '{')) { return PNR_FORMAT_ERROR; }

    pb->msg_end = replylen + 1;

    elem.end = pbjson_find_end_element(reply, reply + replylen);
    /* elem.end has to be just behind end curly brace */
    if ((*reply != '{') || (*(elem.end++) != '}')) {
        PBCC_LOG_ERROR(
            pb->logger_manager,
            "Malformed service response\n  - response: %.*s",
            replylen,
            reply);

        return PNR_FORMAT_ERROR;
    }
    elem.start = reply;

    if (pbjson_value_for_field_found(&elem, "status", "403")) {
        PBCC_LOG_ERROR(
            pb->logger_manager,
            "Token revoke access denied:\n  - response: %.*s",
            replylen,
            reply);
        return PNR_ACCESS_DENIED;
    }

    json_rslt = pbjson_get_object_value(&elem, "data", &parsed);
    if (jonmpKeyNotFound == json_rslt) {
        json_rslt = pbjson_get_object_value(&elem, "error", &parsed);
        if (json_rslt != jonmpOK) {
            PBCC_LOG_ERROR(
                pb->logger_manager,
                "Malformed service response: missing 'error' field\n  - "
                "response: %.*s",
                replylen,
                reply);

            return PNR_FORMAT_ERROR;
        }
        PBCC_LOG_WARNING(
            pb->logger_manager,
            "Service returned error: %.*s",
            (int)(parsed.end - parsed.start),
            parsed.start);

        return PNR_REVOKE_TOKEN_API_ERROR;
    }
    else if (json_rslt != jonmpOK) {
        PBCC_LOG_ERROR(
            pb->logger_manager,
            "Malformed service response: missing 'data' field\n  - response: "
            "%.*s",
            replylen,
            reply);

        return PNR_FORMAT_ERROR;
    }
    PBCC_LOG_TRACE(
        pb->logger_manager,
        "Parsed revoke token data: %.*s",
        (int)(parsed.end - parsed.start),
        parsed.start);

    return PNR_OK;
}
