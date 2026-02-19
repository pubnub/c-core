/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"
#include "pubnub_ccore.h"
#include "pubnub_ccore_pubsub.h"
#include "pubnub_version.h"
#include "pubnub_assert.h"
#include "pubnub_json_parse.h"
#include "pubnub_memory_block.h"
#if PUBNUB_USE_LOGGER
#include "pbcc_logger_manager.h"
#include "pubnub_helper.h"
#endif // PUBNUB_USE_LOGGER
#include "pubnub_server_limits.h"

#include <stdio.h>


static enum pubnub_res simple_parse_response(struct pbcc_context* p)
{
    struct pbjson_elem el;
    char*              reply    = p->http_reply;
    int                replylen = p->http_buf_len;
    if (replylen < 2) { return PNR_FORMAT_ERROR; }

    el.start = reply;
    el.end   = reply + replylen;
    if (pbjson_value_for_field_found(&el, "status", "403")) {
        PBCC_LOG_ERROR(
            p->logger_manager,
            "Access denied:\n  - response: %.*s",
            replylen,
            reply);
        return PNR_ACCESS_DENIED;
    }

    if ((reply[0] != '[') || (reply[replylen - 1] != ']')) {
        return PNR_FORMAT_ERROR;
    }

    p->chan_ofs = p->chan_end = 0;

    p->msg_ofs          = 1;
    p->msg_end          = replylen - 1;
    reply[replylen - 1] = '\0';

    return pbcc_split_array(reply + p->msg_ofs) ? PNR_OK : PNR_FORMAT_ERROR;
}


enum pubnub_res pbcc_parse_time_response(struct pbcc_context* p)
{
    return simple_parse_response(p);
}


enum pubnub_res pbcc_parse_history_response(struct pbcc_context* p)
{
    return simple_parse_response(p);
}


enum pubnub_res pbcc_parse_presence_response(struct pbcc_context* p)
{
    struct pbjson_elem                   el;
    struct pbjson_elem                   parsed;
    enum pbjson_object_name_parse_result json_rslt;
    char*                                reply    = p->http_reply;
    int                                  replylen = p->http_buf_len;
    if (replylen < 2) { return PNR_FORMAT_ERROR; }

    el.start = reply;
    el.end   = reply + replylen;
    if (pbjson_value_for_field_found(&el, "status", "403")) {
        PBCC_LOG_ERROR(
            p->logger_manager,
            "Presence access denied:\n  - response: %.*s",
            replylen,
            reply);
        return PNR_ACCESS_DENIED;
    }

    if ((reply[0] != '{') || (reply[replylen - 1] != '}')) {
        return PNR_FORMAT_ERROR;
    }

    /* Check for error field (typically present when status is 400) */
    json_rslt = pbjson_get_object_value(&el, "error", &parsed);
    if (jonmpOK == json_rslt) {
        /* Server returned an error - try to extract the message */
        struct pbjson_elem msg_elem;
        json_rslt = pbjson_get_object_value(&el, "message", &msg_elem);
        if (jonmpOK == json_rslt) {
            PBCC_LOG_ERROR(
                p->logger_manager,
                "Service returned error: %.*s",
                (int)(msg_elem.end - msg_elem.start),
                msg_elem.start);
        }
        else {
            PBCC_LOG_ERROR(
                p->logger_manager,
                "Service returned error: %.*s",
                replylen,
                reply);
        }
        return PNR_PRESENCE_API_ERROR;
    }

    p->chan_ofs = p->chan_end = 0;

    p->msg_ofs = 0;
    p->msg_end = replylen;

    return PNR_OK;
}


enum pubnub_res pbcc_parse_channel_registry_response(struct pbcc_context* p)
{
    enum pbjson_object_name_parse_result result;
    struct pbjson_elem                   el;
    struct pbjson_elem                   found;

    el.start = p->http_reply;
    el.end   = p->http_reply + p->http_buf_len;
    if (pbjson_value_for_field_found(&el, "status", "403")) {
        PBCC_LOG_ERROR(
            p->logger_manager,
            "Channel registry access denied:\n  - response: %.*s",
            p->http_buf_len,
            p->http_reply);
        return PNR_ACCESS_DENIED;
    }
    p->chan_ofs = 0;
    p->chan_end = p->http_buf_len;

    p->msg_ofs = p->msg_end = 0;

    /* We should probably also check that there is a key "service"
       with value "channel-registry".  Maybe even that there is a key
       "status" (with value 200).
    */
    result = pbjson_get_object_value(&el, "error", &found);
    if (jonmpOK == result) {
        if (pbjson_elem_equals_string(&found, "false")) { return PNR_OK; }
        else {
            return PNR_CHANNEL_REGISTRY_ERROR;
        }
    }
    else {
        return PNR_FORMAT_ERROR;
    }
}


enum pubnub_res pbcc_leave_prep(
    struct pbcc_context* pb,
    const char*          channel,
    const char*          channel_group)
{
    char const* user_id = pbcc_user_id_get(pb);

    PUBNUB_ASSERT_OPT(user_id != NULL);

    enum pubnub_res rslt = PNR_OK;

    if (NULL == channel) {
        if (NULL == channel_group) { return PNR_INVALID_CHANNEL; }
        channel = ",";
    }
    pb->http_content_len = 0;

    /* Make sure next subscribe() will be a join. */
    pb->timetoken[0] = '0';
    pb->timetoken[1] = '\0';

    pb->http_buf_len = snprintf(
        pb->http_buf,
        sizeof pb->http_buf,
        "/v2/presence/sub-key/%s/channel/",
        pb->subscribe_key);
    APPEND_URL_ENCODED_M(pb, channel);
    pb->http_buf_len += snprintf(
        pb->http_buf + pb->http_buf_len,
        sizeof pb->http_buf - pb->http_buf_len,
        "/leave");

    char const* const uname = pubnub_uname();
    URL_PARAMS_INIT(qparam, PUBNUB_MAX_URL_PARAMS);
    if (uname) { ADD_URL_PARAM(qparam, pnsdk, uname); }
    if (user_id) { ADD_URL_PARAM(qparam, uuid, user_id); }
#if PUBNUB_CRYPTO_API
    if (pb->secret_key == NULL) { ADD_URL_AUTH_PARAM(pb, qparam, auth); }
    ADD_TS_TO_URL_PARAM();
#else
    ADD_URL_AUTH_PARAM(pb, qparam, auth);
#endif
    if (channel_group) {
        ADD_URL_PARAM(qparam, channel - group, channel_group);
    }

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


enum pubnub_res pbcc_time_prep(struct pbcc_context* pb)
{
    char const*       user_id = pbcc_user_id_get(pb);
    char const* const uname   = pubnub_uname();

    PUBNUB_ASSERT_OPT(user_id != NULL);

    if (pb->msg_ofs < pb->msg_end) { return PNR_RX_BUFF_NOT_EMPTY; }

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;

    pb->http_buf_len = snprintf(pb->http_buf, sizeof pb->http_buf, "/time/0");

    URL_PARAMS_INIT(qparam, PUBNUB_MAX_URL_PARAMS);
    if (uname) { ADD_URL_PARAM(qparam, pnsdk, uname); }
    if (user_id) { ADD_URL_PARAM(qparam, uuid, user_id); }
    ADD_URL_AUTH_PARAM(pb, qparam, auth);
    ENCODE_URL_PARAMETERS(pb, qparam);

    return PNR_STARTED;
}


enum pubnub_res pbcc_history_prep(
    struct pbcc_context* pb,
    const char*          channel,
    unsigned int         count,
    bool                 include_token,
    enum pubnub_tribool  string_token,
    enum pubnub_tribool  reverse,
    enum pubnub_tribool  include_meta,
    char const*          start,
    char const*          end)
{
    char const* const uname   = pubnub_uname();
    char const*       user_id = pbcc_user_id_get(pb);
    enum pubnub_res   rslt    = PNR_OK;

    PUBNUB_ASSERT_OPT(user_id != NULL);

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;

#if PUBNUB_CRYPTO_API
    for (size_t i = 0; i < pb->decrypted_message_count; i++) {
        free(pb->decrypted_messages[i]);
    }
    pb->decrypted_message_count = 0;
#endif

    pb->http_buf_len = snprintf(
        pb->http_buf,
        sizeof pb->http_buf,
        "/v2/history/sub-key/%s/channel/",
        pb->subscribe_key);
    APPEND_URL_ENCODED_M(pb, channel);

    URL_PARAMS_INIT(qparam, PUBNUB_MAX_URL_PARAMS);
    if (uname) { ADD_URL_PARAM(qparam, pnsdk, uname); }
    if (user_id) { ADD_URL_PARAM(qparam, uuid, user_id); }
    char cnt_buf[sizeof(int) * 4 + 1];
    snprintf(cnt_buf, sizeof(cnt_buf), "%d", count);
    if (count) { ADD_URL_PARAM(qparam, count, cnt_buf); }
    ADD_URL_PARAM(qparam, include_token, include_token ? "true" : "false");
    if (string_token != pbccNotSet) {
        ADD_URL_PARAM(
            qparam, stringtoken, string_token == pbccTrue ? "1" : "0");
    }
#if PUBNUB_CRYPTO_API
    if (pb->secret_key == NULL) { ADD_URL_AUTH_PARAM(pb, qparam, auth); }
    ADD_TS_TO_URL_PARAM();
#else
    ADD_URL_AUTH_PARAM(pb, qparam, auth);
#endif
    if (reverse != pbccNotSet) {
        ADD_URL_PARAM(qparam, reverse, reverse == pbccTrue ? "1" : "0");
    }
    if (include_meta != pbccNotSet) {
        ADD_URL_PARAM(
            qparam, include_meta, include_meta == pbccTrue ? "1" : "0");
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


enum pubnub_res pbcc_heartbeat_prep(
    struct pbcc_context* pb,
    const char*          channel,
    const char*          channel_group)
{
    char const*       user_id = pbcc_user_id_get(pb);
    char const* const uname   = pubnub_uname();
    enum pubnub_res   rslt    = PNR_OK;

    PUBNUB_ASSERT_OPT(user_id != NULL);

    if (NULL == channel) {
        if (NULL == channel_group) { return PNR_INVALID_CHANNEL; }
        channel = ",";
    }
    if (pb->msg_ofs < pb->msg_end) { return PNR_RX_BUFF_NOT_EMPTY; }

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;

    pb->http_buf_len = snprintf(
        pb->http_buf,
        sizeof pb->http_buf,
        "/v2/presence/sub-key/%s/channel/",
        pb->subscribe_key);
    APPEND_URL_ENCODED_M(pb, channel);
    pb->http_buf_len += snprintf(
        pb->http_buf + pb->http_buf_len,
        sizeof pb->http_buf - pb->http_buf_len,
        "/heartbeat");
    URL_PARAMS_INIT(qparam, PUBNUB_MAX_URL_PARAMS);
    if (uname) { ADD_URL_PARAM(qparam, pnsdk, uname); }
    if (channel_group) {
        ADD_URL_PARAM(qparam, channel - group, channel_group);
    }
    if (user_id) { ADD_URL_PARAM(qparam, uuid, user_id); }
    if (pb->state) { ADD_URL_PARAM(qparam, state, pb->state); }
#if PUBNUB_CRYPTO_API
    if (pb->secret_key == NULL) { ADD_URL_AUTH_PARAM(pb, qparam, auth); }
    ADD_TS_TO_URL_PARAM();
#else
    ADD_URL_AUTH_PARAM(pb, qparam, auth);
#endif

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


enum pubnub_res pbcc_here_now_prep(
    struct pbcc_context* pb,
    const char*          channel,
    const char*          channel_group,
    enum pubnub_tribool  disable_uuids,
    enum pubnub_tribool  state,
    unsigned             limit,
    unsigned             offset)
{
    char const* const uname   = pubnub_uname();
    char const*       user_id = pbcc_user_id_get(pb);
    enum pubnub_res   rslt    = PNR_OK;

    PUBNUB_ASSERT_OPT(user_id != NULL);

    if (NULL == channel) {
        if (channel_group != NULL) { channel = ","; }
    }

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;

    pb->http_buf_len = snprintf(
        pb->http_buf,
        sizeof pb->http_buf,
        "/v2/presence/sub-key/%s%s",
        pb->subscribe_key,
        channel ? "/channel/" : "");
    APPEND_URL_ENCODED_M(pb, channel);

    URL_PARAMS_INIT(qparam, PUBNUB_MAX_URL_PARAMS);
    if (uname) { ADD_URL_PARAM(qparam, pnsdk, uname); }
    if (channel_group) {
        ADD_URL_PARAM(qparam, channel - group, channel_group);
    }
    if (user_id) { ADD_URL_PARAM(qparam, uuid, user_id); }
#if PUBNUB_CRYPTO_API
    if (pb->secret_key == NULL) { ADD_URL_AUTH_PARAM(pb, qparam, auth); }
    ADD_TS_TO_URL_PARAM();
#else
    ADD_URL_AUTH_PARAM(pb, qparam, auth);
#endif
    if (disable_uuids != pbccNotSet) {
        ADD_URL_PARAM(
            qparam, disable_uuids, disable_uuids == pbccTrue ? "1" : "0");
    }
    if (state != pbccNotSet) {
        ADD_URL_PARAM(qparam, state, state == pbccTrue ? "1" : "0");
    }


    if (!limit) { limit = PUBNUB_DEFAULT_HERE_NOW_LIMIT; }
    char limit_buf[sizeof(unsigned) * 4 + 1];
    snprintf(limit_buf, sizeof(limit_buf), "%u", limit);
    ADD_URL_PARAM(qparam, limit, limit_buf);

    if (offset) {
        char offset_buf[sizeof(unsigned) * 4 + 1];
        snprintf(offset_buf, sizeof(offset_buf), "%u", offset);
        ADD_URL_PARAM(qparam, offset, offset_buf);
    }

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


enum pubnub_res pbcc_where_now_prep(
    struct pbcc_context* pb,
    const char*          user_id)
{
    PUBNUB_ASSERT_OPT(user_id != NULL);
    enum pubnub_res   rslt       = PNR_OK;
    char const* const uname      = pubnub_uname();
    char const*       pb_user_id = pbcc_user_id_get(pb);

    PUBNUB_ASSERT_OPT(pb_user_id != NULL);

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;

    pb->http_buf_len = snprintf(
        pb->http_buf,
        sizeof pb->http_buf,
        "/v2/presence/sub-key/%s/uuid/",
        pb->subscribe_key);
    APPEND_URL_ENCODED_M(pb, user_id);
    URL_PARAMS_INIT(qparam, PUBNUB_MAX_URL_PARAMS);
    if (uname) { ADD_URL_PARAM(qparam, pnsdk, uname); }
    if (pb_user_id) { ADD_URL_PARAM(qparam, uuid, pb_user_id); }
#if PUBNUB_CRYPTO_API
    if (pb->secret_key == NULL) { ADD_URL_AUTH_PARAM(pb, qparam, auth); }
    ADD_TS_TO_URL_PARAM();
#else
    ADD_URL_AUTH_PARAM(pb, qparam, auth);
#endif

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


enum pubnub_res pbcc_set_state_prep(
    struct pbcc_context* pb,
    char const*          channel,
    char const*          channel_group,
    const char*          user_id,
    char const*          state)
{
    PUBNUB_ASSERT_OPT(user_id != NULL);
    PUBNUB_ASSERT_OPT(state != NULL);
    enum pubnub_res   rslt       = PNR_OK;
    char const*       pb_user_id = pbcc_user_id_get(pb);
    char const* const uname      = pubnub_uname();

    PUBNUB_ASSERT_OPT(pb_user_id != NULL);

    if (NULL == channel) {
        if (NULL == channel_group) { return PNR_INVALID_CHANNEL; }
        channel = ",";
    }
    if (pb->msg_ofs < pb->msg_end) { return PNR_RX_BUFF_NOT_EMPTY; }

    pb->http_buf_len = snprintf(
        pb->http_buf,
        sizeof pb->http_buf,
        "/v2/presence/sub-key/%s/channel/",
        pb->subscribe_key);
    APPEND_URL_ENCODED_M(pb, channel);
    pb->http_buf_len += snprintf(
        pb->http_buf + pb->http_buf_len,
        sizeof pb->http_buf - pb->http_buf_len,
        "/uuid/");
    APPEND_URL_ENCODED_M(pb, user_id);
    APPEND_URL_LITERAL_M(pb, "/data");

    URL_PARAMS_INIT(qparam, PUBNUB_MAX_URL_PARAMS);
    if (uname) { ADD_URL_PARAM(qparam, pnsdk, uname); }
    if (channel_group) {
        ADD_URL_PARAM(qparam, channel - group, channel_group);
    }
    if (pb_user_id) { ADD_URL_PARAM(qparam, uuid, pb_user_id); }
#if PUBNUB_CRYPTO_API
    if (pb->secret_key == NULL) { ADD_URL_AUTH_PARAM(pb, qparam, auth); }
    ADD_TS_TO_URL_PARAM();
#else
    ADD_URL_AUTH_PARAM(pb, qparam, auth);
#endif
    if (state) { ADD_URL_PARAM(qparam, state, state); }

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


enum pubnub_res pbcc_state_get_prep(
    struct pbcc_context* pb,
    char const*          channel,
    char const*          channel_group,
    const char*          user_id)
{
    PUBNUB_ASSERT_OPT(user_id != NULL);
    enum pubnub_res   rslt       = PNR_OK;
    char const*       pb_user_id = pbcc_user_id_get(pb);
    char const* const uname      = pubnub_uname();

    PUBNUB_ASSERT_OPT(pb_user_id != NULL);

    if (NULL == channel) {
        if (NULL == channel_group) { return PNR_INVALID_CHANNEL; }
        channel = ",";
    }
    if (pb->msg_ofs < pb->msg_end) { return PNR_RX_BUFF_NOT_EMPTY; }

    pb->http_buf_len = snprintf(
        pb->http_buf,
        sizeof pb->http_buf,
        "/v2/presence/sub-key/%s/channel/",
        pb->subscribe_key);
    APPEND_URL_ENCODED_M(pb, channel);
    pb->http_buf_len += snprintf(
        pb->http_buf + pb->http_buf_len,
        sizeof pb->http_buf - pb->http_buf_len,
        "/uuid/");
    APPEND_URL_ENCODED_M(pb, user_id);
    URL_PARAMS_INIT(qparam, PUBNUB_MAX_URL_PARAMS);
    if (uname) { ADD_URL_PARAM(qparam, pnsdk, uname); }
    if (channel_group) {
        ADD_URL_PARAM(qparam, channel - group, channel_group);
    }
    if (pb_user_id) { ADD_URL_PARAM(qparam, uuid, pb_user_id); }
#if PUBNUB_CRYPTO_API
    if (pb->secret_key == NULL) { ADD_URL_AUTH_PARAM(pb, qparam, auth); }
    ADD_TS_TO_URL_PARAM();
#else
    ADD_URL_AUTH_PARAM(pb, qparam, auth);
#endif

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


enum pubnub_res pbcc_remove_channel_group_prep(
    struct pbcc_context* pb,
    char const*          channel_group)
{
    PUBNUB_ASSERT_OPT(channel_group != NULL);
    enum pubnub_res   rslt    = PNR_OK;
    char const*       user_id = pbcc_user_id_get(pb);
    char const* const uname   = pubnub_uname();

    PUBNUB_ASSERT_OPT(user_id != NULL);

    pb->http_buf_len = snprintf(
        pb->http_buf,
        sizeof pb->http_buf,
        "/v1/channel-registration/sub-key/%s/channel-group/",
        pb->subscribe_key);
    APPEND_URL_ENCODED_M(pb, channel_group);
    APPEND_URL_LITERAL_M(pb, "/remove");

    URL_PARAMS_INIT(qparam, PUBNUB_MAX_URL_PARAMS);
    if (uname) { ADD_URL_PARAM(qparam, pnsdk, uname); }
    if (user_id) { ADD_URL_PARAM(qparam, uuid, user_id); }
#if PUBNUB_CRYPTO_API
    if (pb->secret_key == NULL) { ADD_URL_AUTH_PARAM(pb, qparam, auth); }
    ADD_TS_TO_URL_PARAM();
#else
    ADD_URL_AUTH_PARAM(pb, qparam, auth);
#endif

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


enum pubnub_res pbcc_channel_registry_prep(
    struct pbcc_context* pb,
    char const*          channel_group,
    char const*          param,
    char const*          channel)
{
    PUBNUB_ASSERT_OPT(channel_group != NULL);
    enum pubnub_res   rslt    = PNR_OK;
    char const*       user_id = pbcc_user_id_get(pb);
    char const* const uname   = pubnub_uname();

    PUBNUB_ASSERT_OPT(user_id != NULL);

    pb->http_buf_len = snprintf(
        pb->http_buf,
        sizeof pb->http_buf,
        "/v1/channel-registration/sub-key/%s/channel-group/",
        pb->subscribe_key);
    APPEND_URL_ENCODED_M(pb, channel_group);

    URL_PARAMS_INIT(qparam, PUBNUB_MAX_URL_PARAMS);
    if (uname) { ADD_URL_PARAM(qparam, pnsdk, uname); }
    if (user_id) { ADD_URL_PARAM(qparam, uuid, user_id); }

    if (NULL != param) {
        PUBNUB_ASSERT_OPT(channel != NULL);
        ADD_URL_PARAM_TRUE_KEY(qparam, param, channel);
    }
#if PUBNUB_CRYPTO_API
    if (pb->secret_key == NULL) { ADD_URL_AUTH_PARAM(pb, qparam, auth); }
    ADD_TS_TO_URL_PARAM();
#else
    ADD_URL_AUTH_PARAM(pb, qparam, auth);
#endif

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
