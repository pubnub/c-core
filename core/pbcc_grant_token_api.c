/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"
#include "pubnub_version.h"
#include "pubnub_assert.h"
#include "pubnub_log.h"
#include "pubnub_helper.h"
#include "pubnub_url_encode.h"
#include "lib/pb_strnlen_s.h"
#include "pbcc_grant_token_api.h"

#include "pbpal.h"
#include <stdio.h>
#include <stdlib.h>

pubnub_chamebl_t pbcc_get_grant_token(struct pbcc_context* pb)
{
    pubnub_chamebl_t result;
    char const* reply = pb->http_reply;
    int replylen = pb->http_buf_len;
    struct pbjson_elem elem;
    struct pbjson_elem parsed;
    enum pbjson_object_name_parse_result json_rslt;
    if (pb->last_result != PNR_OK) {
        PUBNUB_LOG_ERROR("pbcc_get_grant_token(pb=%p) can be called only if "
                         "previous transactin PBTT_GRANT_TOKEN(%d) is finished successfully. "
                         "Transaction result was: %d('%s')\n",
                         pb,
                         PBTT_GRANT_TOKEN,
                         pb->last_result,
                         pubnub_res_2_string(pb->last_result));
        result.ptr = NULL;
        result.size = 0;
        return result;
    }
    
    elem.start = reply;
    elem.end = pbjson_find_end_element(reply, reply + replylen) + 1;
    pbjson_get_object_value(&elem, "data", &parsed);
    json_rslt = pbjson_get_object_value(&parsed, "token", &elem);
    if (json_rslt != jonmpOK) {
        PUBNUB_LOG_ERROR("pbcc_get_grant_token(pbcc=%p) - Invalid response: "
                         "pbjson_get_object_value(\"token\") reported an error: %s\n",
                         pb,
                         pbjson_object_name_parse_result_2_string(json_rslt));

        result.ptr = NULL;
        result.size = 0;
        return result;
    }
    result.size = elem.end - elem.start - 2;
    result.ptr = (char*)elem.start+1;
    result.ptr[result.size] = '\0';

    return result;
}


enum pubnub_res pbcc_grant_token_prep(
    struct pbcc_context* pb,
    const char* perm_obj,
    enum pubnub_trans pt)
{
    char const* const uname = pubnub_uname();
    char const*       user_id = pbcc_user_id_get(pb);
    enum pubnub_res   rslt = PNR_OK;

    PUBNUB_ASSERT_OPT(user_id != NULL);
    PUBNUB_ASSERT_OPT(perm_obj != NULL);

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "/v3/pam/%s/grant",
                                pb->subscribe_key);

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
        rslt = pbcc_sign_url(pb, perm_obj, pubnubSendViaPOST, true);
        if (rslt != PNR_OK) {
            PUBNUB_LOG_ERROR("pbcc_sign_url failed. pb=%p, message=%s, method=%d", pb, perm_obj, pubnubSendViaPOST);
        }
    }
#endif

    APPEND_MESSAGE_BODY_M(rslt, pb, perm_obj);

    PUBNUB_LOG_DEBUG("pbcc_grant_token_prep. REQUEST =%s\n", pb->http_buf);
    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_parse_grant_token_api_response(struct pbcc_context* pb)
{
    char* reply    = pb->http_reply;
    int   replylen = pb->http_buf_len;
    struct pbjson_elem elem;
    struct pbjson_elem parsed;
    enum pbjson_object_name_parse_result json_rslt;

    if ((replylen < 2) || (reply[0] != '{')) {
        return PNR_FORMAT_ERROR;
    }
    
    pb->msg_end = replylen + 1;

    elem.end = pbjson_find_end_element(reply, reply + replylen);
    /* elem.end has to be just behind end curly brace */
    if ((*reply != '{') || (*(elem.end++) != '}')) {
        PUBNUB_LOG_ERROR("pbcc_parse_grant_token_api_response(pbcc=%p) - Invalid: "
                         "response from server is not JSON - response='%s'\n",
                         pb,
                         reply);

        return PNR_FORMAT_ERROR;
    }
    elem.start = reply;

    if (pbjson_value_for_field_found(&elem, "status", "403")){
        PUBNUB_LOG_ERROR("pbcc_parse_grant_token_api_response(pbcc=%p) - AccessDenied: "
                         "response from server - response='%s'\n",
                         pb,
                         reply);
        return PNR_ACCESS_DENIED;
    }

    json_rslt = pbjson_get_object_value(&elem, "data", &parsed);
    if (jonmpKeyNotFound == json_rslt) {
        json_rslt = pbjson_get_object_value(&elem, "error", &parsed);
        if (json_rslt != jonmpOK) {
            PUBNUB_LOG_ERROR("pbcc_parse_grant_token_api_response(pbcc=%p) - Invalid response: "
                             "pbjson_get_object_value(\"error\") reported an error: %s\n",
                             pb,
                             pbjson_object_name_parse_result_2_string(json_rslt));

            return PNR_FORMAT_ERROR;
        }
        PUBNUB_LOG_WARNING("pbcc_parse_grant_token_api_response(pbcc=%p): \"error\"='%.*s'\n",
                           pb,
                           (int)(parsed.end - parsed.start),
                           parsed.start);

        return PNR_GRANT_TOKEN_API_ERROR;
    }
    else if (json_rslt != jonmpOK) {
        PUBNUB_LOG_ERROR("pbcc_parse_grant_token_api_response(pbcc=%p) - Invalid response: "
                         "pbjson_get_object_value(\"data\") reported an error: %s\n",
                         pb,
                         pbjson_object_name_parse_result_2_string(json_rslt));

        return PNR_FORMAT_ERROR;
    }
    PUBNUB_LOG_TRACE("pbcc_parse_grant_token_api_response(pbcc=%p): \"data\"='%.*s'\n",
                     pb,
                     (int)(parsed.end - parsed.start),
                     parsed.start);

    return PNR_OK;
}
