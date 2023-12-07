/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if PUBNUB_USE_ACTIONS_API

#include "pubnub_internal.h"
#include "pubnub_version.h"
#include "pubnub_assert.h"
#include "pubnub_log.h"
#include "pubnub_helper.h"
#include "pubnub_url_encode.h"
#include "pubnub_json_parse.h"
#include "lib/pb_strnlen_s.h"
#include "pbcc_actions_api.h"

#include <stdio.h>
#include <stdlib.h>

/* Maximum number of actions to return in response */
#define MAX_ACTIONS_LIMIT 100


enum pubnub_res pbcc_form_the_action_object(struct pbcc_context* pb,
                                            char* obj_buffer,
                                            size_t buffer_size,
                                            enum pubnub_action_type actype,
                                            char const** val)
{
    char const* user_id = pbcc_user_id_get(pb);
    char const* type_literal;

    PUBNUB_ASSERT_OPT(user_id != NULL);

    if (NULL == user_id) {
        PUBNUB_LOG_ERROR("pbcc_form_the_action_object(pbcc=%p) - user_id not set.\n", pb);
        return PNR_INVALID_PARAMETERS;
    }
    if (('\"' != **val) || ('\"' != *(*val + pb_strnlen_s(*val, PUBNUB_MAX_OBJECT_LENGTH) - 1))) {
        PUBNUB_LOG_ERROR("pbcc_form_the_action_object(pbcc=%p) - "
                         "quotation marks on value ends are missing: "
                         "value = %s\n",
                         pb,
                         *val);
        return PNR_INVALID_PARAMETERS;
    }
    switch(actype) {
    case pbactypReaction:
        type_literal = "reaction";
        break;
    case pbactypReceipt:
        type_literal = "receipt";
        break;
    case pbactypCustom:
        type_literal = "custom";
        break;
    default:
        PUBNUB_LOG_ERROR("pbcc_form_the_action_object(pbcc=%p) - "
                         "unknown action type = %d\n",
                         pb,
                         actype);
        return PNR_INVALID_PARAMETERS;
    }
    if (buffer_size < sizeof("{\"type\":\"\",\"value\":,\"user_id\":\"\"}") +
                             pb_strnlen_s(type_literal, sizeof "reaction") +
                             pb_strnlen_s(*val, PUBNUB_MAX_OBJECT_LENGTH) +
                             pb->user_id_len) {
        PUBNUB_LOG_ERROR("pbcc_form_the_action_object(pbcc=%p) - "
                         "buffer size is too small: "
                         "current_buffer_size = %lu\n"
                         "required_buffer_size = %lu\n",
                         pb,
                         (unsigned long)buffer_size,
                         (unsigned long)(sizeof("{\"type\":\"\",\"value\":,\"user_id\":\"\"}") +
                                         pb_strnlen_s(type_literal, sizeof "reaction") +
                                         pb_strnlen_s(*val, PUBNUB_MAX_OBJECT_LENGTH) +
                                         pb->user_id_len));
        return PNR_TX_BUFF_TOO_SMALL;
    }
    snprintf(obj_buffer,
             buffer_size,
             "{\"type\":\"%s\",\"value\":%s,\"user_id\":\"%s\"}",
             type_literal,
             *val,
             user_id);
    *val = obj_buffer;
    
    return PNR_OK;
}


enum pubnub_res pbcc_add_action_prep(struct pbcc_context* pb,
                                     char const* channel, 
                                     char const* message_timetoken, 
                                     char const* value)
{
    char const* const uname = pubnub_uname();
    char const*       user_id = pbcc_user_id_get(pb);
    enum pubnub_res   rslt = PNR_OK;

    PUBNUB_ASSERT_OPT(user_id != NULL);
    PUBNUB_ASSERT_OPT(channel != NULL);
    PUBNUB_ASSERT_OPT(message_timetoken != NULL);
    PUBNUB_ASSERT_OPT(value != NULL);

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
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
        if (rslt != PNR_OK) {
            PUBNUB_LOG_ERROR("pbcc_sign_url failed. pb=%p, message=%s, method=%d", pb, value, pubnubSendViaPOST);
        }
    }
#endif

    APPEND_MESSAGE_BODY_M(PNR_OK, pb, value);

    PUBNUB_LOG_DEBUG("pbcc_add_action_prep. REQUEST =%s\n", pb->http_buf);
    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


pubnub_chamebl_t pbcc_get_message_timetoken(struct pbcc_context* pb)
{
    pubnub_chamebl_t result;
    char const* reply = pb->http_reply;
    int replylen = pb->http_buf_len;
    struct pbjson_elem elem;
    struct pbjson_elem parsed;
    enum pbjson_object_name_parse_result json_rslt;
    if (pb->last_result != PNR_OK) {
        PUBNUB_LOG_ERROR("pbcc_get_message_timetoken(pb=%p) can be called only if "
                         "previous transactin PBTT_ADD_ACTION(%d) is finished successfully. "
                         "Transaction result was: %d('%s')\n",
                         pb,
                         PBTT_ADD_ACTION,
                         pb->last_result,
                         pubnub_res_2_string(pb->last_result));
        result.ptr = NULL;
        result.size = 0;
        return result;
    }
    
    elem.start = reply;
    elem.end = pbjson_find_end_element(reply, reply + replylen) + 1;
    pbjson_get_object_value(&elem, "data", &parsed);
    json_rslt = pbjson_get_object_value(&parsed, "messageTimetoken", &elem);
    if (json_rslt != jonmpOK) {
        PUBNUB_LOG_ERROR("pbcc_get_message_timetoken(pbcc=%p) - Invalid response: "
                         "pbjson_get_object_value(\"messageTimetoken\") reported an error: %s\n",
                         pb,
                         pbjson_object_name_parse_result_2_string(json_rslt));

        result.ptr = NULL;
        result.size = 0;
        return result;
    }
    result.ptr = (char*)elem.start;
    result.size = elem.end - elem.start;

    return result;
}


pubnub_chamebl_t pbcc_get_action_timetoken(struct pbcc_context* pb)
{
    pubnub_chamebl_t result;
    char const* reply = pb->http_reply;
    int replylen = pb->http_buf_len;
    struct pbjson_elem elem;
    struct pbjson_elem parsed;
    enum pbjson_object_name_parse_result json_rslt;
    if (pb->last_result != PNR_OK) {
        PUBNUB_LOG_ERROR("pbcc_get_action_timetoken(pb=%p) can be called only if "
                         "previous transaction PBTT_ADD_ACTION(%d) is finished successfully. "
                         "Transaction result was: %d('%s')\n",
                         pb,
                         PBTT_ADD_ACTION,
                         pb->last_result,
                         pubnub_res_2_string(pb->last_result));
        result.ptr = NULL;
        result.size = 0;
        return result;
    }
    
    elem.start = reply;
    elem.end = pbjson_find_end_element(reply, reply + replylen) + 1;
    pbjson_get_object_value(&elem, "data", &parsed);
    json_rslt = pbjson_get_object_value(&parsed, "actionTimetoken", &elem);
    if (json_rslt != jonmpOK) {
        PUBNUB_LOG_ERROR("pbcc_get_action_timetoken(pbcc=%p) - Invalid response: "
                         "pbjson_get_object_value(\"actionTimetoken\") reported an error: %s\n",
                         pb,
                         pbjson_object_name_parse_result_2_string(json_rslt));

        result.ptr = NULL;
        result.size = 0;
        return result;
    }
    result.ptr = (char*)elem.start;
    result.size = elem.end - elem.start;

    return result;
}


enum pubnub_res pbcc_remove_action_prep(struct pbcc_context* pb,
                                        char const* channel,
                                        pubnub_chamebl_t message_timetoken,
                                        pubnub_chamebl_t action_timetoken)
{
    char const* const uname = pubnub_uname();
    char const*       user_id = pbcc_user_id_get(pb);
    enum pubnub_res   rslt = PNR_OK;

    PUBNUB_ASSERT_OPT(user_id != NULL);
    PUBNUB_ASSERT_OPT(channel != NULL);
    PUBNUB_ASSERT_OPT(message_timetoken.ptr != NULL);
    PUBNUB_ASSERT_OPT(action_timetoken.ptr != NULL);

    if ((*message_timetoken.ptr != '\"') ||
        (*(message_timetoken.ptr + message_timetoken.size - 1) != '\"')) {
        PUBNUB_LOG_ERROR("pbcc_remove_action_prep(pbcc=%p) - "
                         "message timetoken is missing quotation marks: "
                         "message_timetoken = %.*s\n",
                         pb,
                         (int)message_timetoken.size,
                         message_timetoken.ptr);
        return PNR_INVALID_PARAMETERS;
    }
    if ((*action_timetoken.ptr != '\"') ||
        (*(action_timetoken.ptr + action_timetoken.size - 1) != '\"')) {
        PUBNUB_LOG_ERROR("pbcc_remove_action_prep(pbcc=%p) - "
                         "action timetoken is missing quotation marks: "
                         "action_timetoken = %.*s\n",
                         pb,
                         (int)action_timetoken.size,
                         action_timetoken.ptr);
        return PNR_INVALID_PARAMETERS;
    }

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "/v1/message-actions/%s/channel/",
                                pb->subscribe_key);
    APPEND_URL_ENCODED_M(pb, channel);
    APPEND_URL_LITERAL_M(pb, "/message/");
    APPEND_URL_STRING_M(pb, message_timetoken.ptr + 1, message_timetoken.size - 2);
    APPEND_URL_LITERAL_M(pb, "/action/");
    APPEND_URL_STRING_M(pb, action_timetoken.ptr + 1, action_timetoken.size - 2);

    URL_PARAMS_INIT(qparam, PUBNUB_MAX_URL_PARAMS);
    if (uname) { ADD_URL_PARAM(qparam, pnsdk, uname); }
    if (user_id) { ADD_URL_PARAM(qparam, uuid, user_id); }
#if PUBNUB_CRYPTO_API
    if (pb->secret_key == NULL){
        ADD_URL_AUTH_PARAM(pb, qparam, auth);
    }
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
        if (rslt != PNR_OK) {
            PUBNUB_LOG_ERROR("pbcc_sign_url failed. pb=%p, message=%s, method=%d", pb, "", pubnubUseDELETE);
        }
    }
#endif
    
    PUBNUB_LOG_DEBUG("pbcc_remove_action_prep. REQUEST =%s\n", pb->http_buf);
    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_get_actions_prep(struct pbcc_context* pb,
                                      char const* channel,
                                      char const* start,
                                      char const* end,
                                      size_t limit)
{
    char const* const uname = pubnub_uname();
    char const*       user_id = pbcc_user_id_get(pb);
    enum pubnub_res   rslt = PNR_OK;

    PUBNUB_ASSERT_OPT(user_id != NULL);
    PUBNUB_ASSERT_OPT(channel != NULL);
    PUBNUB_ASSERT_OPT(limit <= MAX_ACTIONS_LIMIT);

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
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
    if (limit != 0) { ADD_URL_PARAM_SIZET(qparam, limit, limit); }

#if PUBNUB_CRYPTO_API
  SORT_URL_PARAMETERS(qparam);
#endif
    ENCODE_URL_PARAMETERS(pb, qparam);
#if PUBNUB_CRYPTO_API
    if (pb->secret_key != NULL) {
        rslt = pbcc_sign_url(pb, "", pubnubSendViaGET, true);
        if (rslt != PNR_OK) {
            PUBNUB_LOG_ERROR("pbcc_sign_url failed. pb=%p, message=%s, method=%d", pb, "", pubnubSendViaGET);
        }
    }
#endif

    PUBNUB_LOG_DEBUG("pbcc_get_actions_prep. REQUEST =%s\n", pb->http_buf);
    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_get_actions_more_prep(struct pbcc_context* pb)
{
    enum pubnub_res rslt = PNR_OK;
    char const* const uname = pubnub_uname();
    char const*       user_id = pbcc_user_id_get(pb);
    char const* reply = pb->http_reply;
    int replylen = pb->http_buf_len;
    struct pbjson_elem elem;
    struct pbjson_elem parsed;
    enum pbjson_object_name_parse_result json_rslt;

    PUBNUB_ASSERT_OPT(user_id != NULL);

    if (pb->last_result != PNR_OK) {
        PUBNUB_LOG_ERROR("pbcc_get_actions_more_prep(pb=%p) can be called only if "
                         "previous transaction is finished successfully. "
                         "Transaction result was: %d('%s')\n",
                         pb,
                         pb->last_result,
                         pubnub_res_2_string(pb->last_result));

        return PNR_INTERNAL_ERROR;
    }
    
    elem.start = reply;
    elem.end = pbjson_find_end_element(reply, reply + replylen) + 1;
    json_rslt = pbjson_get_object_value(&elem, "more", &parsed);
    if (jonmpKeyNotFound == json_rslt) {
        return PNR_GOT_ALL_ACTIONS;
    }
    else if (json_rslt != jonmpOK) {
        PUBNUB_LOG_ERROR("pbcc_get_actions_more_prep(pbcc=%p) - Invalid response: "
                         "pbjson_get_object_value(\"more\") reported an error: %s\n",
                         pb,
                         pbjson_object_name_parse_result_2_string(json_rslt));

        return PNR_FORMAT_ERROR;
    }
    json_rslt = pbjson_get_object_value(&parsed, "url", &elem);
    if (json_rslt != jonmpOK) {
        PUBNUB_LOG_ERROR("pbcc_get_actions_more_prep(pbcc=%p) - Invalid response: "
                         "pbjson_get_object_value(\"url\") reported an error: %s\n",
                         pb,
                         pbjson_object_name_parse_result_2_string(json_rslt));

        return PNR_FORMAT_ERROR;
    }
    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
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
        if (rslt != PNR_OK) {
            PUBNUB_LOG_ERROR("pbcc_sign_url failed. pb=%p, message=%s, method=%d", pb, "", pubnubSendViaGET);
        }
    }
#endif

    PUBNUB_LOG_DEBUG("pbcc_get_actions_more_prep. REQUEST =%s\n", pb->http_buf);
    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_history_with_actions_prep(struct pbcc_context* pb,
                                               char const* channel,
                                               char const* start,
                                               char const* end,
                                               size_t limit)
{
    char const* const uname = pubnub_uname();
    char const*       user_id = pbcc_user_id_get(pb);
    enum pubnub_res rslt = PNR_OK;

    PUBNUB_ASSERT_OPT(user_id != NULL);
    PUBNUB_ASSERT_OPT(channel != NULL);
    PUBNUB_ASSERT_OPT(limit <= MAX_ACTIONS_LIMIT);

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
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
    if (limit != 0) { ADD_URL_PARAM_SIZET(qparam, limit, limit); }
    
#if PUBNUB_CRYPTO_API
  SORT_URL_PARAMETERS(qparam);
#endif
    ENCODE_URL_PARAMETERS(pb, qparam);
#if PUBNUB_CRYPTO_API
    if (pb->secret_key != NULL) {
        rslt = pbcc_sign_url(pb, "", pubnubSendViaGET, true);
        if (rslt != PNR_OK) {
            PUBNUB_LOG_ERROR("pbcc_sign_url failed. pb=%p, message=%s, method=%d", pb, "", pubnubSendViaGET);
        }
    }
#endif

    PUBNUB_LOG_DEBUG("pbcc_history_with_actions_prep. REQUEST =%s\n", pb->http_buf);
    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_parse_actions_api_response(struct pbcc_context* pb)
{
    char const* reply = pb->http_reply;
    int replylen = pb->http_buf_len;
    struct pbjson_elem elem;
    struct pbjson_elem parsed;
    enum pbjson_object_name_parse_result json_rslt;

    if ((replylen < 2) || (reply[0] != '{')) {
        return PNR_FORMAT_ERROR;
    }

    pb->chan_ofs = pb->chan_end = 0;
    pb->msg_ofs = 0;
    pb->msg_end = replylen + 1;
    
    elem.end = pbjson_find_end_element(reply, reply + replylen);
    /* elem.end has to be just behind end curly brace */
    if ((*reply != '{') || (*(elem.end++) != '}')) {
        PUBNUB_LOG_ERROR("pbcc_parse_actions_api_response(pbcc=%p) - Invalid: "
                         "response from server is not JSON - response='%s'\n",
                         pb,
                         reply);

        return PNR_FORMAT_ERROR;
    }
    elem.start = reply;
    if (pbjson_value_for_field_found(&elem, "status", "403")) {
        PUBNUB_LOG_ERROR("pbcc_parse_actions_api_response(pbcc=%p) - AccessDenied: "
                         "response from server - response='%s'\n",
                         pb,
                         reply);
        return PNR_ACCESS_DENIED;
    }
    json_rslt = pbjson_get_object_value(&elem, "data", &parsed);
    if (jonmpKeyNotFound == json_rslt) {
        json_rslt = pbjson_get_object_value(&elem, "error", &parsed);
        if (json_rslt != jonmpOK) {
            PUBNUB_LOG_ERROR("pbcc_parse_actions_api_response(pbcc=%p) - Invalid response: "
                             "pbjson_get_object_value(\"error\") reported an error: %s\n",
                             pb,
                             pbjson_object_name_parse_result_2_string(json_rslt));

            return PNR_FORMAT_ERROR;
        }
        PUBNUB_LOG_WARNING("pbcc_parse_actions_api_response(pbcc=%p): \"error\"='%.*s'\n",
                           pb,
                           (int)(parsed.end - parsed.start),
                           parsed.start);
        
        return PNR_ACTIONS_API_ERROR;
    }
    else if (json_rslt != jonmpOK) {
        PUBNUB_LOG_ERROR("pbcc_parse_actions_api_response(pbcc=%p) - Invalid response: "
                         "pbjson_get_object_value(\"data\") reported an error: %s\n",
                         pb,
                         pbjson_object_name_parse_result_2_string(json_rslt));

        return PNR_FORMAT_ERROR;
    }
    PUBNUB_LOG_TRACE("pbcc_parse_actions_api_response(pbcc=%p): \"data\"='%.*s'\n",
                     pb,
                     (int)(parsed.end - parsed.start),
                     parsed.start);

    return PNR_OK;
}


enum pubnub_res pbcc_parse_history_with_actions_response(struct pbcc_context* pb)
{
    char const* reply = pb->http_reply;
    int replylen = pb->http_buf_len;
    struct pbjson_elem elem;
    struct pbjson_elem parsed;
    enum pbjson_object_name_parse_result json_rslt;

    if ((replylen < 2) || (reply[0] != '{')) {
        return PNR_FORMAT_ERROR;
    }

    pb->chan_ofs = pb->chan_end = 0;
    pb->msg_ofs = 0;
    pb->msg_end = replylen + 1;
    
    elem.end = pbjson_find_end_element(reply, reply + replylen);
    /* elem.end has to be just behind end curly brace */
    if ((*reply != '{') || (*(elem.end++) != '}')) {
        PUBNUB_LOG_ERROR("pbcc_parse_history_with_actions_response(pbcc=%p) - Invalid: "
                         "response from server is not JSON - response='%s'\n",
                         pb,
                         reply);

        return PNR_FORMAT_ERROR;
    }
    elem.start = reply;
    if (pbjson_value_for_field_found(&elem, "status", "403")) {
        PUBNUB_LOG_ERROR("pbcc_parse_history_with_actions_response(pbcc=%p) - AccessDenied: "
                         "response from server - response='%s'\n",
                         pb,
                         reply);
        return PNR_ACCESS_DENIED;
    }
    json_rslt = pbjson_get_object_value(&elem, "channels", &parsed);
    if (jonmpKeyNotFound == json_rslt) {
        json_rslt = pbjson_get_object_value(&elem, "error_message", &parsed);
        if (json_rslt != jonmpOK) {
            PUBNUB_LOG_ERROR("pbcc_parse_history_with_actions_response(pbcc=%p) - Invalid response: "
                             "pbjson_get_object_value(\"error_message\") reported an error: %s\n",
                             pb,
                             pbjson_object_name_parse_result_2_string(json_rslt));

            return PNR_FORMAT_ERROR;
        }
        PUBNUB_LOG_WARNING("pbcc_parse_history_with_actions_response(pbcc=%p): "
                           "\"error_message\"='%.*s'\n",
                           pb,
                           (int)(parsed.end - parsed.start),
                           parsed.start);
        
        return PNR_ACTIONS_API_ERROR;
    }
    else if (json_rslt != jonmpOK) {
        PUBNUB_LOG_ERROR("pbcc_parse_history_with_actions_response(pbcc=%p) - Invalid response: "
                         "pbjson_get_object_value(\"channels\") reported an error: %s\n",
                         pb,
                         pbjson_object_name_parse_result_2_string(json_rslt));

        return PNR_FORMAT_ERROR;
    }
    PUBNUB_LOG_TRACE("pbcc_parse_history_with_actions_response(pbcc=%p): \"channels\"='%.*s'\n",
                     pb,
                     (int)(parsed.end - parsed.start),
                     parsed.start);

    return PNR_OK;
}

#endif /* PUBNUB_USE_ACTIONS_API */
