/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#if PUBNUB_USE_FETCH_HISTORY
#include "pubnub_memory_block.h"
#include "pubnub_server_limits.h"
#include "pubnub_fetch_history.h"
#include "pubnub_version.h"
#include "pubnub_json_parse.h"
#include "pubnub_url_encode.h"

#include "pubnub_assert.h"
#include "pubnub_helper.h"
#if PUBNUB_USE_LOGGER
#include "pbcc_logger_manager.h"
#endif // PUBNUB_USE_LOGGER
#else
#error this module can only be used if PUBNUB_USE_FETCH_HISTORY is defined and set to 1
#endif


enum pubnub_res pbcc_fetch_history_prep(
    struct pbcc_context* pb,
    const char*          channel,
    unsigned int         max_per_channel,
    enum pubnub_tribool  include_meta,
    enum pubnub_tribool  include_custom_message_type,
    enum pubnub_tribool  include_message_type,
    enum pubnub_tribool  include_user_id,
    enum pubnub_tribool  include_message_actions,
    enum pubnub_tribool  reverse,
    char const*          start,
    char const*          end)
{
    char const* const uname = pubnub_uname();
    enum pubnub_res   rslt  = PNR_OK;

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;

    pb->http_buf_len = snprintf(
        pb->http_buf,
        sizeof pb->http_buf,
        "/v3/%s/sub-key/%s/channel/",
        (include_message_actions == pbccTrue) ? "history-with-actions"
                                              : "history",
        pb->subscribe_key);
    APPEND_URL_ENCODED_M(pb, channel);

    URL_PARAMS_INIT(qparam, PUBNUB_MAX_URL_PARAMS);
    if (uname) { ADD_URL_PARAM(qparam, pnsdk, uname); }

    int   ch_count = 0;
    char* ch_lst   = (char*)strtok((char*)channel, ",");
    while (ch_lst != NULL) {
        ch_count++;
        ch_lst = (char*)strtok(NULL, ",");
    }
    if (max_per_channel <= 0) {
        if (include_message_actions == pbccTrue || ch_count > 1) {
            max_per_channel = 25;
        }
        else {
            max_per_channel = 100;
        }
    }
    char max_per_ch_cnt_buf[sizeof(int) * 4 + 1];
    snprintf(
        max_per_ch_cnt_buf, sizeof(max_per_ch_cnt_buf), "%d", max_per_channel);
    if (max_per_channel) { ADD_URL_PARAM(qparam, max, max_per_ch_cnt_buf); }

    if (include_meta != pbccNotSet) {
        ADD_URL_PARAM(
            qparam, include_meta, include_meta == pbccTrue ? "true" : "false");
    }
    if (include_custom_message_type != pbccNotSet) {
        ADD_URL_PARAM(
            qparam,
            include_custom_message_type,
            include_custom_message_type == pbccTrue ? "true" : "false");
    }
    if (include_message_type != pbccNotSet) {
        ADD_URL_PARAM(
            qparam,
            include_message_type,
            include_message_type == pbccTrue ? "true" : "false");
    }
    if (include_user_id != pbccNotSet) {
        ADD_URL_PARAM(
            qparam,
            include_uuid,
            include_user_id == pbccTrue ? "true" : "false");
    }
#if PUBNUB_CRYPTO_API
    if (pb->secret_key == NULL) { ADD_URL_AUTH_PARAM(pb, qparam, auth); }
    ADD_TS_TO_URL_PARAM();
#else
    ADD_URL_AUTH_PARAM(pb, qparam, auth);
#endif
    if (reverse != pbccNotSet) {
        ADD_URL_PARAM(qparam, reverse, reverse == pbccTrue ? "true" : "false");
    }
    if (start) { ADD_URL_PARAM(qparam, start, start); }
    if (end) { ADD_URL_PARAM(qparam, end, end); }

#if PUBNUB_CRYPTO_API
    SORT_URL_PARAMETERS(qparam);
#endif
    ENCODE_URL_PARAMETERS(pb, qparam);
#if PUBNUB_CRYPTO_API
    if (pb->secret_key != NULL) {
        rslt = pbcc_sign_url(pb, "", pubnubSendViaGET, true);
    }
#endif

    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}

enum pubnub_res pbcc_parse_fetch_history_response(struct pbcc_context* pb)
{
    char*              reply    = pb->http_reply;
    int                replylen = pb->http_buf_len;
    struct pbjson_elem elem;

    PBCC_LOG_TRACE(
        pb->logger_manager, "Parsing fetch history service response...");

    if ((replylen < 2) || (reply[0] != '{')) { return PNR_FORMAT_ERROR; }

    pb->msg_end = replylen + 1;

    elem.end = pbjson_find_end_element(reply, reply + replylen);
    /* elem.end has to be just behind end curly brace */
    if ((*reply != '{') || (*(elem.end++) != '}')) {
        PBCC_LOG_ERROR(
            pb->logger_manager,
            "Malformed service response:\n  - response: %.*s",
            replylen,
            reply);

        return PNR_FORMAT_ERROR;
    }
    elem.start = reply;

    if (pbjson_value_for_field_found(&elem, "status", "403")) {
        PBCC_LOG_ERROR(
            pb->logger_manager,
            "Fetch history access denied:\n  - response: %.*s",
            replylen,
            reply);
        return PNR_ACCESS_DENIED;
    }

    if (pbjson_value_for_field_found(&elem, "error", "true")) {
        PBCC_LOG_ERROR(
            pb->logger_manager,
            "Service returned error:\n  - response: %.*s",
            replylen,
            reply);
        return PNR_FETCH_HISTORY_ERROR;
    }

    return PNR_OK;
}

pubnub_chamebl_t pbcc_get_fetch_history(struct pbcc_context* pb)
{
    pubnub_chamebl_t   result;
    char const*        reply    = pb->http_reply;
    int                replylen = pb->http_buf_len;
    struct pbjson_elem elem;
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

    elem.start = reply;
    elem.end   = pbjson_find_end_element(reply, reply + replylen) + 1;

    result.size             = elem.end - elem.start;
    result.ptr              = (char*)elem.start;
    result.ptr[result.size] = '\0';

    return result;
}
