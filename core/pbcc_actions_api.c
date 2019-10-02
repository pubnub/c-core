/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
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
                                            char const** json)
{
    char const* uuid = pbcc_uuid_get(pb);
    char const* type_literal;
    if (NULL == uuid) {
        PUBNUB_LOG_ERROR("pbcc_form_the_action_object(pbcc=%p) - uuid not set.\n", pb);
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
    if (buffer_size < sizeof("{\"type\":\"\",\"value\":,\"uuid\":\"\"}") +
                             pb_strnlen_s(type_literal, sizeof "reaction") +
                             pb_strnlen_s(*json, PUBNUB_MAX_OBJECT_LENGTH) +
                             pb_strnlen_s(uuid, sizeof pb->uuid)) {
        PUBNUB_LOG_ERROR("pbcc_form_the_action_object(pbcc=%p) - "
                         "buffer size is too small: "
                         "current_buffer_size = %lu\n"
                         "required_buffer_size = %lu\n",
                         pb,
                         (unsigned long)buffer_size,
                         (unsigned long)(sizeof("{\"type\":\"\",\"value\":,\"uuid\":\"\"}") +
                                         pb_strnlen_s(type_literal, sizeof "reaction") +
                                         pb_strnlen_s(*json, PUBNUB_MAX_OBJECT_LENGTH) +
                                         pb_strnlen_s(uuid, sizeof pb->uuid)));
        return PNR_TX_BUFF_TOO_SMALL;
    }
    snprintf(obj_buffer,
             buffer_size,
             "{\"type\":\"%s\",\"value\":%s,\"uuid\":\"%s\"}",
             type_literal,
             *json,
             uuid);
    *json = obj_buffer;
    
    return PNR_OK;
}


enum pubnub_res pbcc_add_action_prep(struct pbcc_context* pb,
                                     char const* channel, 
                                     char const* message_timetoken, 
                                     char const* value)
{
    char const* const uname = pubnub_uname();
    char const*       uuid = pbcc_uuid_get(pb);

    PUBNUB_ASSERT_OPT(channel != NULL);
    PUBNUB_ASSERT_OPT(message_timetoken != NULL);
    PUBNUB_ASSERT_OPT(value != NULL);

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "/v1/actions/%s/channel/",
                                pb->subscribe_key);
    APPEND_URL_ENCODED_M(pb, channel);
    APPEND_URL_LITERAL_M(pb, "/message/");
    APPEND_URL_ENCODED_M(pb, message_timetoken);
    APPEND_URL_PARAM_M(pb, "pnsdk", uname, '?');
    APPEND_URL_PARAM_M(pb, "uuid", uuid, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');
    APPEND_MESSAGE_BODY_M(PNR_OK, pb, value);

    return PNR_STARTED;
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
                                        char const* message_timetoken,
                                        char const* action_timetoken)
{
    char const* const uname = pubnub_uname();
    char const*       uuid = pbcc_uuid_get(pb);

    PUBNUB_ASSERT_OPT(channel != NULL);
    PUBNUB_ASSERT_OPT(message_timetoken != NULL);
    PUBNUB_ASSERT_OPT(action_timetoken != NULL);

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "/v1/actions/%s/channel/",
                                pb->subscribe_key);
    APPEND_URL_ENCODED_M(pb, channel);
    APPEND_URL_LITERAL_M(pb, "/message/");
    APPEND_URL_ENCODED_M(pb, message_timetoken);
    APPEND_URL_LITERAL_M(pb, "/action/");
    APPEND_URL_ENCODED_M(pb, action_timetoken);
    APPEND_URL_PARAM_M(pb, "pnsdk", uname, '?');
    APPEND_URL_PARAM_M(pb, "uuid", uuid, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');

    return PNR_STARTED;
}


enum pubnub_res pbcc_get_actions_prep(struct pbcc_context* pb,
                                      char const* channel,
                                      char const* start,
                                      char const* end,
                                      size_t limit)
{
    char const* const uname = pubnub_uname();
    char const*       uuid = pbcc_uuid_get(pb);

    PUBNUB_ASSERT_OPT(channel != NULL);
    PUBNUB_ASSERT_OPT(limit < MAX_ACTIONS_LIMIT);

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "/v1/actions/%s/channel/",
                                pb->subscribe_key);
    APPEND_URL_ENCODED_M(pb, channel);
    APPEND_URL_PARAM_M(pb, "pnsdk", uname, '?');
    APPEND_URL_PARAM_M(pb, "uuid", uuid, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');
    APPEND_URL_PARAM_M(pb, "start", start, '&');
    APPEND_URL_PARAM_M(pb, "end", end, '&');
    APPEND_URL_PARAM_UNSIGNED_M(pb, "limit", (unsigned)limit, '&');

    return PNR_STARTED;
}


enum pubnub_res pbcc_get_actions_more_prep(struct pbcc_context* pb)
{
    char const* const uname = pubnub_uname();
    char const*       uuid = pbcc_uuid_get(pb);
    char const* reply = pb->http_reply;
    int replylen = pb->http_buf_len;
    struct pbjson_elem elem;
    struct pbjson_elem parsed;
    enum pbjson_object_name_parse_result json_rslt;
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
    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "%.*s",
                                (int)(parsed.end - parsed.start - 2),
                                parsed.start + 1);
    APPEND_URL_PARAM_M(pb, "pnsdk", uname, '&');
    APPEND_URL_PARAM_M(pb, "uuid", uuid, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');

    return PNR_STARTED;
}


enum pubnub_res pbcc_history_with_actions_prep(struct pbcc_context* pb,
                                               char const* channel,
                                               char const* start,
                                               char const* end,
                                               size_t limit)
{
    char const* const uname = pubnub_uname();
    char const*       uuid = pbcc_uuid_get(pb);

    PUBNUB_ASSERT_OPT(channel != NULL);
    PUBNUB_ASSERT_OPT(limit < MAX_ACTIONS_LIMIT);

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "/v1/history-with-actions/%s/channel/",
                                pb->subscribe_key);
    APPEND_URL_ENCODED_M(pb, channel);
    APPEND_URL_PARAM_M(pb, "pnsdk", uname, '?');
    APPEND_URL_PARAM_M(pb, "uuid", uuid, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');
    APPEND_URL_PARAM_M(pb, "start", start, '&');
    APPEND_URL_PARAM_M(pb, "end", end, '&');
    APPEND_URL_PARAM_UNSIGNED_M(pb, "limit", (unsigned)limit, '&');

    return PNR_STARTED;
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
