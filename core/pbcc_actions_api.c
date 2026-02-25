/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if PUBNUB_USE_ACTIONS_API

#include "pubnub_internal.h"
#include "pubnub_version.h"
#include "pubnub_assert.h"
#include "pubnub_helper.h"

#if PUBNUB_USE_LOGGER
#include "pbcc_logger_manager.h"
#endif // PUBNUB_USE_LOGGER
#include "pubnub_url_encode.h"
#include "pubnub_json_parse.h"
#include "lib/pb_strnlen_s.h"
#include "pbcc_actions_api.h"

#include <stdio.h>
#include <stdlib.h>

/* Maximum number of actions to return in response */
#define MAX_ACTIONS_LIMIT 100
#define MAX_ACTION_TYPE_LENGTH 15


enum pubnub_res pbcc_form_the_action_object(
    struct pbcc_context*    pb,
    char*                   obj_buffer,
    size_t                  buffer_size,
    enum pubnub_action_type actype,
    char const**            val)
{
    char const* type_literal;

    switch (actype) {
    case pbactypReaction:
        type_literal = "\"reaction\"";
        break;
    case pbactypReceipt:
        type_literal = "\"receipt\"";
        break;
    case pbactypCustom:
        type_literal = "\"custom\"";
        break;
    default:
        PBCC_LOG_ERROR(
            pb->logger_manager, "Unknown message action type: %d", actype);
        return PNR_INVALID_PARAMETERS;
    }

    return pbcc_form_the_action_object_str(
        pb, obj_buffer, buffer_size, type_literal, val);
}


enum pubnub_res pbcc_form_the_action_object_str(
    struct pbcc_context* pb,
    char*                obj_buffer,
    size_t               buffer_size,
    char const*          action_type,
    char const**         val)
{
    char const* user_id = pbcc_user_id_get(pb);

    PUBNUB_ASSERT_OPT(user_id != NULL);

    if (NULL == user_id) {
        PBCC_LOG_ERROR(pb->logger_manager, "'user_id' is missing");
        return PNR_INVALID_PARAMETERS;
    }
    if (('\"' != **val) ||
        ('\"' != *(*val + pb_strnlen_s(*val, PUBNUB_MAX_OBJECT_LENGTH) - 1))) {
        PBCC_LOG_ERROR(
            pb->logger_manager,
            "Message action 'value' is missing quotes\n  - value: %s",
            *val);
        return PNR_INVALID_PARAMETERS;
    }
    if (('\"' != *action_type) ||
        ('\"' != *(action_type +
                   pb_strnlen_s(action_type, MAX_ACTION_TYPE_LENGTH) - 1))) {
        PBCC_LOG_ERROR(
            pb->logger_manager,
            "Message action 'type' is missing quotes\n  - type: %s",
            action_type);
        return PNR_INVALID_PARAMETERS;
    }

    size_t required_buffer_size =
        sizeof("{\"type\":\"\",\"value\":,\"user_"
               "id\":\"\"}") +
        pb_strnlen_s(action_type, MAX_ACTION_TYPE_LENGTH) +
        pb_strnlen_s(*val, PUBNUB_MAX_OBJECT_LENGTH) + pb->user_id_len;
    if (buffer_size < required_buffer_size) {
        PBCC_LOG_ERROR(
            pb->logger_manager,
            "Buffer is too small: %lu bytes needed, %lu bytes available.",
            (unsigned long)required_buffer_size,
            (unsigned long)buffer_size);
        return PNR_TX_BUFF_TOO_SMALL;
    }
    snprintf(
        obj_buffer,
        buffer_size,
        "{\"type\":%s,\"value\":%s,\"user_id\":\"%s\"}",
        action_type,
        *val,
        user_id);
    *val = obj_buffer;

    return PNR_OK;
}


enum pubnub_res pbcc_add_action_prep(
    struct pbcc_context* pb,
    char const*          channel,
    char const*          message_timetoken,
    char const*          value)
{
    char const* const uname = pbcc_uname(pb);
    char const*       user_id = pbcc_user_id_get(pb);
    enum pubnub_res   rslt    = PNR_OK;

    PUBNUB_ASSERT_OPT(user_id != NULL);
    PUBNUB_ASSERT_OPT(channel != NULL);
    PUBNUB_ASSERT_OPT(message_timetoken != NULL);
    PUBNUB_ASSERT_OPT(value != NULL);

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len          = snprintf(
        pb->http_buf,
        sizeof pb->http_buf,
        "/v1/message-actions/%s/channel/",
        pb->subscribe_key);
    APPEND_URL_ENCODED_M(pb, channel);
    APPEND_URL_LITERAL_M(pb, "/message/");
    APPEND_URL_ENCODED_M(pb, message_timetoken);

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
        rslt = pbcc_sign_url(pb, value, pubnubSendViaPOST, true);
#if PUBNUB_LOG_ENABLED(ERROR)
        if (rslt != PNR_OK) {
            pbcc_logger_manager_log_error(
                pb->logger_manager,
                PUBNUB_LOG_LEVEL_ERROR,
                PUBNUB_LOG_LOCATION,
                rslt,
                "Add message action URL signing failed",
                pubnub_res_2_string(rslt));
        }
#endif // PUBNUB_LOG_ENABLED(ERROR)
    }
#endif

    APPEND_MESSAGE_BODY_M(PNR_OK, pb, value);

    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


pubnub_chamebl_t pbcc_get_message_timetoken(struct pbcc_context* pb)
{
    pubnub_chamebl_t                     result;
    char const*                          reply    = pb->http_reply;
    int                                  replylen = pb->http_buf_len;
    struct pbjson_elem                   elem;
    struct pbjson_elem                   parsed;
    enum pbjson_object_name_parse_result json_rslt;
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
    pbjson_get_object_value(&elem, "data", &parsed);
    json_rslt = pbjson_get_object_value(&parsed, "messageTimetoken", &elem);
    if (json_rslt != jonmpOK) {
        PBCC_LOG_ERROR(
            pb->logger_manager,
            "Malformed service response: missing 'messageTimetoken' "
            "field\n  - response: %.*s",
            replylen,
            reply);

        result.ptr  = NULL;
        result.size = 0;
        return result;
    }
    result.ptr  = (char*)elem.start;
    result.size = elem.end - elem.start;

    return result;
}


pubnub_chamebl_t pbcc_get_action_timetoken(struct pbcc_context* pb)
{
    pubnub_chamebl_t                     result;
    char const*                          reply    = pb->http_reply;
    int                                  replylen = pb->http_buf_len;
    struct pbjson_elem                   elem;
    struct pbjson_elem                   parsed;
    enum pbjson_object_name_parse_result json_rslt;
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
    pbjson_get_object_value(&elem, "data", &parsed);
    json_rslt = pbjson_get_object_value(&parsed, "actionTimetoken", &elem);
    if (json_rslt != jonmpOK) {
        PBCC_LOG_ERROR(
            pb->logger_manager,
            "Malformed service response: missing 'actionTimetoken' "
            "field\n  - response: %.*s",
            replylen,
            reply);

        result.ptr  = NULL;
        result.size = 0;
        return result;
    }
    result.ptr  = (char*)elem.start;
    result.size = elem.end - elem.start;

    return result;
}


enum pubnub_res pbcc_remove_action_prep(
    struct pbcc_context* pb,
    char const*          channel,
    pubnub_chamebl_t     message_timetoken,
    pubnub_chamebl_t     action_timetoken)
{
    char const* const uname = pbcc_uname(pb);
    char const*       user_id = pbcc_user_id_get(pb);
    enum pubnub_res   rslt    = PNR_OK;

    PUBNUB_ASSERT_OPT(user_id != NULL);
    PUBNUB_ASSERT_OPT(channel != NULL);
    PUBNUB_ASSERT_OPT(message_timetoken.ptr != NULL);
    PUBNUB_ASSERT_OPT(action_timetoken.ptr != NULL);

    if ((*message_timetoken.ptr != '\"') ||
        (*(message_timetoken.ptr + message_timetoken.size - 1) != '\"')) {
        PBCC_LOG_ERROR(
            pb->logger_manager, "Message timetoken is missing quotes");
        return PNR_INVALID_PARAMETERS;
    }
    if ((*action_timetoken.ptr != '\"') ||
        (*(action_timetoken.ptr + action_timetoken.size - 1) != '\"')) {
        PBCC_LOG_ERROR(
            pb->logger_manager, "Message action timetoken is missing quotes");
        return PNR_INVALID_PARAMETERS;
    }

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len          = snprintf(
        pb->http_buf,
        sizeof pb->http_buf,
        "/v1/message-actions/%s/channel/",
        pb->subscribe_key);
    APPEND_URL_ENCODED_M(pb, channel);
    APPEND_URL_LITERAL_M(pb, "/message/");
    APPEND_URL_STRING_M(
        pb, message_timetoken.ptr + 1, message_timetoken.size - 2);
    APPEND_URL_LITERAL_M(pb, "/action/");
    APPEND_URL_STRING_M(
        pb, action_timetoken.ptr + 1, action_timetoken.size - 2);

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
        rslt = pbcc_sign_url(pb, "", pubnubUseDELETE, true);
#if PUBNUB_LOG_ENABLED(ERROR)
        if (rslt != PNR_OK) {
            pbcc_logger_manager_log_error(
                pb->logger_manager,
                PUBNUB_LOG_LEVEL_ERROR,
                PUBNUB_LOG_LOCATION,
                rslt,
                "Remove message action URL signing failed",
                pubnub_res_2_string(rslt));
        }
#endif // PUBNUB_LOG_ENABLED(ERROR)
    }
#endif

    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_get_actions_prep(
    struct pbcc_context* pb,
    char const*          channel,
    char const*          start,
    char const*          end,
    size_t               limit)
{
    char const* const uname = pbcc_uname(pb);
    char const*       user_id = pbcc_user_id_get(pb);
    enum pubnub_res   rslt    = PNR_OK;

    PUBNUB_ASSERT_OPT(user_id != NULL);
    PUBNUB_ASSERT_OPT(channel != NULL);
    PUBNUB_ASSERT_OPT(limit <= MAX_ACTIONS_LIMIT);

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len          = snprintf(
        pb->http_buf,
        sizeof pb->http_buf,
        "/v1/message-actions/%s/channel/",
        pb->subscribe_key);
    APPEND_URL_ENCODED_M(pb, channel);

    URL_PARAMS_INIT(qparam, PUBNUB_MAX_URL_PARAMS);
    if (uname) { ADD_URL_PARAM(qparam, pnsdk, uname); }
    if (user_id) { ADD_URL_PARAM(qparam, uuid, user_id); }
#if PUBNUB_CRYPTO_API
    if (pb->secret_key == NULL) { ADD_URL_AUTH_PARAM(pb, qparam, auth); }
    ADD_TS_TO_URL_PARAM();
#else
    ADD_URL_AUTH_PARAM(pb, qparam, auth);
#endif
    if (start) { ADD_URL_PARAM(qparam, start, start); }
    if (end) { ADD_URL_PARAM(qparam, end, end); }
    if (limit != 0) { ADD_URL_PARAM_SIZET(pb, qparam, limit, limit); }

#if PUBNUB_CRYPTO_API
    SORT_URL_PARAMETERS(qparam);
#endif
    ENCODE_URL_PARAMETERS(pb, qparam);
#if PUBNUB_CRYPTO_API
    if (pb->secret_key != NULL) {
        rslt = pbcc_sign_url(pb, "", pubnubSendViaGET, true);
#if PUBNUB_LOG_ENABLED(ERROR)
        if (rslt != PNR_OK) {
            pbcc_logger_manager_log_error(
                pb->logger_manager,
                PUBNUB_LOG_LEVEL_ERROR,
                PUBNUB_LOG_LOCATION,
                rslt,
                "Get message actions URL signing failed",
                pubnub_res_2_string(rslt));
        }
#endif // PUBNUB_LOG_ENABLED(ERROR)
    }
#endif

    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_get_actions_more_prep(struct pbcc_context* pb)
{
    enum pubnub_res                      rslt     = PNR_OK;
    char const* const                    uname    = pbcc_uname(pb);
    char const*                          user_id  = pbcc_user_id_get(pb);
    char const*                          reply    = pb->http_reply;
    int                                  replylen = pb->http_buf_len;
    struct pbjson_elem                   elem;
    struct pbjson_elem                   parsed;
    enum pbjson_object_name_parse_result json_rslt;

    PUBNUB_ASSERT_OPT(user_id != NULL);

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
        return PNR_INTERNAL_ERROR;
    }

    elem.start = reply;
    elem.end   = pbjson_find_end_element(reply, reply + replylen) + 1;
    json_rslt  = pbjson_get_object_value(&elem, "more", &parsed);
    if (jonmpKeyNotFound == json_rslt) { return PNR_GOT_ALL_ACTIONS; }
    else if (json_rslt != jonmpOK) {
        PBCC_LOG_ERROR(
            pb->logger_manager,
            "Malformed service response: 'more' field is missing\n  - "
            "response: %.*s",
            replylen,
            reply);
        return PNR_FORMAT_ERROR;
    }
    json_rslt = pbjson_get_object_value(&parsed, "url", &elem);
    if (json_rslt != jonmpOK) {
        PBCC_LOG_ERROR(
            pb->logger_manager,
            "Malformed service response: 'url' field is missing\n  - response: "
            "%.*s",
            replylen,
            reply);
        return PNR_FORMAT_ERROR;
    }
    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len          = snprintf(
        pb->http_buf,
        sizeof pb->http_buf,
        "%.*s",
        (int)(elem.end - elem.start - 2),
        elem.start + 1);
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
#if PUBNUB_LOG_ENABLED(ERROR)
        if (rslt != PNR_OK) {
            pbcc_logger_manager_log_error(
                pb->logger_manager,
                PUBNUB_LOG_LEVEL_ERROR,
                PUBNUB_LOG_LOCATION,
                rslt,
                "Get more message actions URL signing failed",
                pubnub_res_2_string(rslt));
        }
#endif // PUBNUB_LOG_ENABLED(ERROR)
    }
#endif

    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_history_with_actions_prep(
    struct pbcc_context* pb,
    char const*          channel,
    char const*          start,
    char const*          end,
    size_t               limit)
{
    char const* const uname = pbcc_uname(pb);
    char const*       user_id = pbcc_user_id_get(pb);
    enum pubnub_res   rslt    = PNR_OK;

    PUBNUB_ASSERT_OPT(user_id != NULL);
    PUBNUB_ASSERT_OPT(channel != NULL);
    PUBNUB_ASSERT_OPT(limit <= MAX_ACTIONS_LIMIT);

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len          = snprintf(
        pb->http_buf,
        sizeof pb->http_buf,
        "/v3/history-with-actions/sub-key/%s/channel/",
        pb->subscribe_key);
    APPEND_URL_ENCODED_M(pb, channel);

    URL_PARAMS_INIT(qparam, PUBNUB_MAX_URL_PARAMS);
    if (uname) { ADD_URL_PARAM(qparam, pnsdk, uname); }
    if (user_id) { ADD_URL_PARAM(qparam, uuid, user_id); }
#if PUBNUB_CRYPTO_API
    if (pb->secret_key == NULL) { ADD_URL_AUTH_PARAM(pb, qparam, auth); }
    ADD_TS_TO_URL_PARAM();
#else
    ADD_URL_AUTH_PARAM(pb, qparam, auth);
#endif
    if (start) { ADD_URL_PARAM(qparam, start, start); }
    if (end) { ADD_URL_PARAM(qparam, end, end); }
    if (limit != 0) { ADD_URL_PARAM_SIZET(pb, qparam, limit, limit); }

#if PUBNUB_CRYPTO_API
    SORT_URL_PARAMETERS(qparam);
#endif
    ENCODE_URL_PARAMETERS(pb, qparam);
#if PUBNUB_CRYPTO_API
    if (pb->secret_key != NULL) {
        rslt = pbcc_sign_url(pb, "", pubnubSendViaGET, true);
#if PUBNUB_LOG_ENABLED(ERROR)
        if (rslt != PNR_OK) {
            pbcc_logger_manager_log_error(
                pb->logger_manager,
                PUBNUB_LOG_LEVEL_ERROR,
                PUBNUB_LOG_LOCATION,
                rslt,
                "History with message actions URL signing failed",
                pubnub_res_2_string(rslt));
        }
#endif // PUBNUB_LOG_ENABLED(ERROR)
    }
#endif

    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_parse_actions_api_response(struct pbcc_context* pb)
{
    char const*                          reply    = pb->http_reply;
    int                                  replylen = pb->http_buf_len;
    struct pbjson_elem                   elem;
    struct pbjson_elem                   parsed;
    enum pbjson_object_name_parse_result json_rslt;

    PBCC_LOG_TRACE(
        pb->logger_manager, "Parsing message actions service response...");

    if ((replylen < 2) || (reply[0] != '{')) {
        PBCC_LOG_ERROR(
            pb->logger_manager,
            "Malformed service response\n  - response: %.*s",
            replylen,
            reply);
        return PNR_FORMAT_ERROR;
    }

    pb->chan_ofs = pb->chan_end = 0;
    pb->msg_ofs                 = 0;
    pb->msg_end                 = replylen + 1;

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
            "Access denied\n  - response: %.*s",
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
                "Malformed service response: 'error' field is "
                "missing\n  - response: %.*s",
                replylen,
                reply);
            return PNR_FORMAT_ERROR;
        }
        PBCC_LOG_ERROR(
            pb->logger_manager,
            "Service returned error: %.*s",
            (int)(parsed.end - parsed.start),
            parsed.start);
        return PNR_ACTIONS_API_ERROR;
    }
    else if (json_rslt != jonmpOK) {
        PBCC_LOG_ERROR(
            pb->logger_manager,
            "Malformed service response: 'data' field is missing\n  - "
            "response: %.*s",
            replylen,
            reply);
        return PNR_FORMAT_ERROR;
    }
    PBCC_LOG_TRACE(
        pb->logger_manager,
        "Message actions response parsed\n  - data: %.*s",
        (int)(parsed.end - parsed.start),
        parsed.start);

    return PNR_OK;
}


enum pubnub_res pbcc_parse_history_with_actions_response(
    struct pbcc_context* pb)
{
    char const*                          reply    = pb->http_reply;
    int                                  replylen = pb->http_buf_len;
    struct pbjson_elem                   elem;
    struct pbjson_elem                   parsed;
    enum pbjson_object_name_parse_result json_rslt;

    PBCC_LOG_TRACE(
        pb->logger_manager,
        "Parsing history with message actions service response...");

    if ((replylen < 2) || (reply[0] != '{')) {
        PBCC_LOG_ERROR(
            pb->logger_manager,
            "Malformed service response\n  - response: %.*s",
            replylen,
            reply);
        return PNR_FORMAT_ERROR;
    }

    pb->chan_ofs = pb->chan_end = 0;
    pb->msg_ofs                 = 0;
    pb->msg_end                 = replylen + 1;

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
            "Access denied\n  - response: %.*s",
            replylen,
            reply);
        return PNR_ACCESS_DENIED;
    }
    json_rslt = pbjson_get_object_value(&elem, "channels", &parsed);
    if (jonmpKeyNotFound == json_rslt) {
        json_rslt = pbjson_get_object_value(&elem, "error_message", &parsed);
        if (json_rslt != jonmpOK) {
            PBCC_LOG_ERROR(
                pb->logger_manager,
                "Malformed service response: 'error_message' field "
                "is missing\n  - response: %.*s",
                replylen,
                reply);
            return PNR_FORMAT_ERROR;
        }
        PBCC_LOG_ERROR(
            pb->logger_manager,
            "Service returned error: %.*s",
            (int)(parsed.end - parsed.start),
            parsed.start);
        return PNR_ACTIONS_API_ERROR;
    }
    else if (json_rslt != jonmpOK) {
        PBCC_LOG_ERROR(
            pb->logger_manager,
            "Malformed service response: 'channels' field is "
            "missing\n  - response: %.*s",
            replylen,
            reply);
        return PNR_FORMAT_ERROR;
    }
    PBCC_LOG_TRACE(
        pb->logger_manager,
        "History with message actions parsed\n  - data: %.*s",
        (int)(parsed.end - parsed.start),
        parsed.start);

    return PNR_OK;
}
#endif /* PUBNUB_USE_ACTIONS_API */
