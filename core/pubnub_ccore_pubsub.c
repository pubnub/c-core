/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbcc_crypto.h"
#include "pubnub_internal.h"
#include "pubnub_version.h"
#include "pubnub_assert.h"
#include "pubnub_json_parse.h"
#include "pubnub_log.h"
#include "pubnub_url_encode.h"
#include "lib/pb_strnlen_s.h"
#include "pubnub_ccore_pubsub.h"
#include "pubnub_api_types.h"
#include <string.h>

#if PUBNUB_CRYPTO_API
#include "pubnub_crypto.h"
#endif

#if PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE
#include "core/pbcc_subscribe_event_engine.h"
#endif // PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE

#include <stdio.h>
#include <stdlib.h>


void pbcc_init(struct pbcc_context* p,
               const char*          publish_key,
               const char*          subscribe_key)
{
    p->publish_key   = publish_key;
    p->subscribe_key = subscribe_key;
    p->timetoken[0]  = '0';
    p->timetoken[1]  = '\0';
    p->user_id       = NULL;
    p->user_id_len   = 0;
    p->auth          = NULL;
    p->auth_token    = NULL;
    p->msg_ofs       = p->msg_end = 0;
#if PUBNUB_DYNAMIC_REPLY_BUFFER
    p->http_reply = NULL;
#if PUBNUB_RECEIVE_GZIP_RESPONSE
    p->decomp_buf_size   = (size_t)0;
    p->decomp_http_reply = NULL;
#endif /* PUBNUB_RECEIVE_GZIP_RESPONSE */
#endif /* PUBNUB_DYNAMIC_REPLY_BUFFER */
    p->message_to_send = NULL;
    p->state           = NULL;
#if PUBNUB_USE_RETRY_CONFIGURATION
    p->retry_configuration = NULL;
    p->http_retry_count    = 0;
    p->retry_timer         = NULL;
#endif // #if PUBNUB_USE_RETRY_CONFIGURATION
#if PUBNUB_USE_GZIP_COMPRESSION
    p->gzip_msg_len = 0;
#endif
#if PUBNUB_CRYPTO_API
    p->secret_key = NULL;
    p->crypto_module = NULL;
    p->decrypted_message_count = 0;
#endif
}


void pbcc_deinit(struct pbcc_context* p)
{
    pbcc_set_user_id(p, NULL);
#if PUBNUB_DYNAMIC_REPLY_BUFFER
    if (p->http_reply != NULL) {
        free(p->http_reply);
        p->http_reply = NULL;
    }
#if PUBNUB_RECEIVE_GZIP_RESPONSE
    if (p->decomp_http_reply != NULL) {
        free(p->decomp_http_reply);
        p->decomp_http_reply = NULL;
    }
#endif /* PUBNUB_RECEIVE_GZIP_RESPONSE */
#endif /* PUBNUB_DYNAMIC_REPLY_BUFFER */
#if PUBNUB_USE_RETRY_CONFIGURATION
    if (NULL != p->retry_configuration)
        pubnub_retry_configuration_free(&p->retry_configuration);
    p->http_retry_count = 0;
    if (NULL != p->retry_timer) {
        pbcc_request_retry_timer_stop(p->retry_timer);
        pbcc_request_retry_timer_free(&p->retry_timer);
    }
#endif // #if PUBNUB_USE_RETRY_CONFIGURATION
#if PUBNUB_CRYPTO_API
    if (NULL != p->crypto_module) {
        free(p->crypto_module);
        p->crypto_module = NULL;
    }
#endif /* PUBNUB_CRYPTO_API */
#if PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE
    pbcc_subscribe_ee_free(&p->subscribe_ee);
#endif // PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE
#if PUBNUB_USE_RETRY_CONFIGURATION
    if (NULL != p->retry_configuration)
        pubnub_retry_configuration_free(&p->retry_configuration);
#endif // #if PUBNUB_USE_RETRY_CONFIGURATION
}


int pbcc_realloc_reply_buffer(struct pbcc_context* p, unsigned bytes)
{
#if PUBNUB_DYNAMIC_REPLY_BUFFER
    char* newbuf = (char*)realloc(p->http_reply, bytes + 1);
    if (NULL == newbuf) {
        return -1;
    }
    p->http_reply = newbuf;
    return 0;
#else
    if (bytes < sizeof p->http_reply / sizeof p->http_reply[0]) {
        return 0;
    }
    return -1;
#endif
}


bool pbcc_ensure_reply_buffer(struct pbcc_context* p)
{
#if PUBNUB_DYNAMIC_REPLY_BUFFER
    if (NULL == p->http_reply) {
        /* Need just one byte for string end */
        p->http_reply = (char*)malloc(1);
        if (NULL == p->http_reply) {
            return false;
        }
    }
#endif
    return true;
}


char const* pbcc_get_msg(struct pbcc_context* pb)
{
#if PUBNUB_CRYPTO_API
    size_t len;
#endif // PUBNUB_CRYPTO_API
    if (pb->msg_ofs < pb->msg_end) {
        PUBNUB_LOG_DEBUG("RESPONSE = %s\n", pb->http_reply);
        char const* rslt = pb->http_reply + pb->msg_ofs;
        pb->msg_ofs += strlen(rslt);
        if (pb->msg_ofs++ <= pb->msg_end) {
#if PUBNUB_CRYPTO_API
            if (pb->crypto_module != NULL) {
                len = strlen(rslt);
                rslt = pbcc_decrypt_message(pb, rslt, len, NULL);
            }
#endif // PUBNUB_CRYPTO_API
            return rslt;
        }
    }

    return NULL;
}


char const* pbcc_get_channel(struct pbcc_context* pb)
{
    if (pb->chan_ofs < pb->chan_end) {
        char const* rslt = pb->http_reply + pb->chan_ofs;
        pb->chan_ofs += strlen(rslt);
        if (pb->chan_ofs++ <= pb->chan_end) {
            return rslt;
        }
    }

    return NULL;
}


enum pubnub_res pbcc_set_user_id(struct pbcc_context* pb, const char* user_id)
{
    if (pb->user_id_len > 0) {
        free(pb->user_id);
        pb->user_id = NULL;
    }
    pb->user_id_len = NULL == user_id ? 0 : strlen(user_id);
    if (pb->user_id_len > 0) {
        /** Alloc additional space for NULL character */
        pb->user_id = (char*)malloc((pb->user_id_len + 1) * sizeof(char));
        if (NULL == pb->user_id) {
            PUBNUB_LOG_ERROR(
                "Error: pbcc_set_user_id(pb=%p) - "
                "Failed to allocate memory for user_id: "
                "user_id = '%s'\n",
                pb,
                user_id);
            pb->user_id_len = 0;
            return PNR_OUT_OF_MEMORY;
        }
        strcpy(pb->user_id, user_id);
    }
    return PNR_OK;
}


char const* pbcc_user_id_get(struct pbcc_context* pb)
{
    return (0 == pb->user_id_len) ? NULL : pb->user_id;
}


void pbcc_set_auth(struct pbcc_context* pb, const char* auth)
{
    pb->auth = auth;
}

void pbcc_set_auth_token(struct pbcc_context* pb, const char* token)
{
    pb->auth_token = token;
}

/* Find the beginning of a JSON string that comes after comma and ends
 * at @c &buf[len].
 * @return position (index) of the found start or -1 on error. */
static int find_string_start(char const* buf, int len)
{
    int i;
    for (i = len - 1; i > 0; --i) {
        if (buf[i] == '"') {
            return (buf[i - 1] == ',') ? i : -1;
        }
    }
    return -1;
}


bool pbcc_split_array(char* buf)
{
    bool escaped       = false;
    bool in_string     = false;
    int  bracket_level = 0;

    for (; *buf != '\0'; ++buf) {
        if (escaped) {
            escaped = false;
        }
        else if ('"' == *buf) {
            in_string = !in_string;
        }
        else if (in_string) {
            escaped = ('\\' == *buf);
        }
        else {
            switch (*buf) {
            case '[':
            case '{':
                bracket_level++;
                break;
            case ']':
            case '}':
                bracket_level--;
                break;
            /* if at root, split! */
            case ',':
                if (bracket_level == 0) {
                    *buf = '\0';
                }
                break;
            default:
                break;
            }
        }
    }

    return !(escaped || in_string || (bracket_level > 0));
}


enum pubnub_res pbcc_parse_publish_response(struct pbcc_context* p)
{
    struct pbjson_elem el;
    char*              reply = p->http_reply;
    PUBNUB_LOG_DEBUG("\n%s\n", reply);
    int replylen = p->http_buf_len;
    if (replylen < 2) {
        return PNR_FORMAT_ERROR;
    }

    el.start = reply;
    el.end   = reply + replylen;
    if (pbjson_value_for_field_found(&el, "status", "403")) {
        PUBNUB_LOG_ERROR("pbcc_parse_publish_response(pbcc=%p) - AccessDenied: "
                         "response from server - response='%s'\n",
                         p,
                         reply);
        return PNR_ACCESS_DENIED;
    }

    p->chan_ofs = p->chan_end = 0;
    p->msg_ofs  = p->msg_end  = 0;

    if ((reply[0] != '[') || (reply[replylen - 1] != ']')) {
        if (reply[0] != '{') {
            return PNR_FORMAT_ERROR;
        }
        /* If we got a JSON object in response, publish certainly
           didn't succeed. No need to parse further. */
        return PNR_PUBLISH_FAILED;
    }

    reply[replylen - 1] = '\0';
    if (pbcc_split_array(reply + 1)) {
        if (1 != strtol(reply + 1, NULL, 10)) {
            return PNR_PUBLISH_FAILED;
        }
        return PNR_OK;
    }
    else {
        return PNR_FORMAT_ERROR;
    }
}


enum pubnub_res pbcc_parse_subscribe_response(struct pbcc_context* p)
{
    struct pbjson_elem el;
    int                i;
    int                previous_i;
    unsigned           time_token_length;
    char*              reply = p->http_reply;
    PUBNUB_LOG_DEBUG("\n%s\n", reply);
    int replylen = p->http_buf_len;
    if (replylen < 2) {
        return PNR_FORMAT_ERROR;
    }
    el.start = reply;
    el.end   = reply + replylen;
    if (pbjson_value_for_field_found(&el, "status", "403")) {
        PUBNUB_LOG_ERROR(
            "pbcc_parse_subscribe_response(pbcc=%p) - AccessDenied: "
            "response from server - response='%s'\n",
            p,
            reply);
        return PNR_ACCESS_DENIED;
    }

    if (pbjson_value_for_field_found(&el, "status", "400")) {
        char* msgtext = (char*)pbjson_get_status_400_message_value(&el);
        if (msgtext != NULL && strcmp(msgtext,
                                      "\"Channel group or groups result in empty subscription set\"")
            == 0) {
            return PNR_GROUP_EMPTY;
        }
        else {
            return PNR_FORMAT_ERROR;
        }
    }

    if (reply[replylen - 1] != ']' && replylen > 2) {
        replylen -= 2; /* XXX: this seems required by Manxiang */
    }
    if ((reply[0] != '[') || (reply[replylen - 1] != ']')
        || (reply[replylen - 2] != '"')) {
        return PNR_FORMAT_ERROR;
    }

    /* Extract the last argument. */
    previous_i = replylen - 2;
    i          = find_string_start(reply, previous_i);
    if (i < 0) {
        return PNR_FORMAT_ERROR;
    }
    reply[replylen - 2] = 0;

    /* Now, the last argument may either be a timetoken, a channel group list
       or a channel list. */
    if (reply[i - 2] == '"') {
        int k;
        /* It is a channel list, there is another string argument in front
         * of us. Process the channel list ... */
        for (k = replylen - 2; k > i + 1; --k) {
            if (reply[k] == ',') {
                reply[k] = '\0';
            }
        }

        /* The previous argument is either a timetoken or a channel group
           list. */
        reply[i - 2] = '\0';
        p->chan_ofs  = i + 1;
        p->chan_end  = replylen - 1;
        previous_i   = i - 2;
        i            = find_string_start(reply, previous_i);
        if (i < 0) {
            p->chan_ofs = 0;
            p->chan_end = 0;
            return PNR_FORMAT_ERROR;
        }
        if (reply[i - 2] == '"') {
            /* It is a channel group list. For now, we shall skip
               it. In the future, we may process it like we do the
               channel list.
            */
            reply[i - 2] = '\0';
            previous_i   = i - 2;
            i            = find_string_start(reply, previous_i);
            if (i < 0) {
                return PNR_FORMAT_ERROR;
            }
        }
    }
    else {
        p->chan_ofs = 0;
        p->chan_end = 0;
    }

    /* Now, `i` points to:
     * [[1,2,3],"5678"]
     * [[1,2,3],"5678","a,b,c"]
     * [[1,2,3],"5678","gr-a,gr-b,gr-c","a,b,c"]
     *          ^-- here */

    /* Setup timetoken. */
    time_token_length = previous_i - (i + 1);
    if (time_token_length >= sizeof p->timetoken) {
        p->timetoken[0] = '\0';
        return PNR_FORMAT_ERROR;
    }
    memcpy(p->timetoken, reply + i + 1, time_token_length + 1);

    /* terminate the [] message array (before the `]`!) */
    reply[i - 2] = 0;

    /* Set up the message list - offset, length and NUL-characters
     * splitting the messages. */
    p->msg_ofs = 2;
    p->msg_end = i - 2;

    return pbcc_split_array(reply + p->msg_ofs) ? PNR_OK : PNR_FORMAT_ERROR;
}


enum pubnub_res pbcc_append_url_param(struct pbcc_context* pb,
                                      char const*          param_name,
                                      size_t               param_name_len,
                                      char const*          param_val,
                                      char                 separator)
{
    size_t param_val_len = strlen(param_val);
    if (pb->http_buf_len + 1 + param_name_len + 1 + param_val_len + 1
        > sizeof pb->http_buf) {
        return PNR_TX_BUFF_TOO_SMALL;
    }

    pb->http_buf[pb->http_buf_len++] = separator;
    memcpy(pb->http_buf + pb->http_buf_len, param_name, param_name_len);
    pb->http_buf_len += param_name_len;
    pb->http_buf[pb->http_buf_len++] = '=';
    memcpy(pb->http_buf + pb->http_buf_len, param_val, param_val_len + 1);
    pb->http_buf_len += param_val_len;

    return PNR_OK;
}


void pbcc_via_post_headers(struct pbcc_context* pb,
                           char*                header,
                           size_t               max_length)
{
    char     lines[] = "Content-Type: application/json\r\nContent-Length: ";
    unsigned length;

    PUBNUB_ASSERT_OPT(pb != NULL);
    PUBNUB_ASSERT_OPT(pb->message_to_send != NULL);
    PUBNUB_ASSERT_OPT(header != NULL);
    PUBNUB_ASSERT_OPT(max_length > sizeof lines);
    memcpy(header, lines, sizeof lines - 1);
    header += sizeof lines - 1;
    max_length -= sizeof lines - 1;
#if PUBNUB_USE_GZIP_COMPRESSION
    if (pb->gzip_msg_len != 0) {
        char h_encoding[] = "Content-Encoding: gzip\0";
        length            = snprintf(header,
                          max_length,
                          "%lu\r\n",
                          (unsigned long)pb->gzip_msg_len);
        PUBNUB_ASSERT_OPT(max_length > length + sizeof h_encoding - 1);
        memcpy(header + length, h_encoding, sizeof h_encoding - 1);
        return;
    }
#endif
    length = snprintf(header,
                      max_length,
                      "%lu",
                      (unsigned long)pb_strnlen_s(
                          pb->message_to_send,
                          PUBNUB_MAX_OBJECT_LENGTH));
    PUBNUB_ASSERT_OPT(max_length > length);
}


enum pubnub_res pbcc_url_encode(struct pbcc_context* pb,
                                char const*          what,
                                enum pubnub_trans    pt)
{
    int url_encoded_length;
    url_encoded_length = pubnub_url_encode(pb->http_buf + pb->http_buf_len,
                                           what,
                                           sizeof pb->http_buf - pb->
                                           http_buf_len,
                                           pt);
    if (url_encoded_length < 0) {
        pb->http_buf_len = 0;
        return PNR_TX_BUFF_TOO_SMALL;
    }
    pb->http_buf_len += url_encoded_length;

    return PNR_OK;
}


enum pubnub_res pbcc_append_url_param_encoded(struct pbcc_context* pb,
                                              char const* param_name,
                                              size_t param_name_len,
                                              char const* param_val,
                                              char separator,
                                              enum pubnub_trans pt)
{
    if (pb->http_buf_len + 1 + param_name_len + 1 > sizeof pb->http_buf) {
        return PNR_TX_BUFF_TOO_SMALL;
    }

    pb->http_buf[pb->http_buf_len++] = separator;
    memcpy(pb->http_buf + pb->http_buf_len, param_name, param_name_len);
    pb->http_buf_len += param_name_len;
    pb->http_buf[pb->http_buf_len++] = '=';
    return pbcc_url_encode(pb, param_val, pt);
}

enum pubnub_res pbcc_publish_prep(struct pbcc_context* pb,
                                  const char*          channel,
                                  const char*          message,
                                  const char*          custom_message_type,
                                  bool                 store_in_history,
                                  bool                 norep,
                                  char const*          meta,
                                  const size_t         ttl,
                                  enum pubnub_method   method)
{
    char const* const uname   = pubnub_uname();
    char const*       user_id = pbcc_user_id_get(pb);
    enum pubnub_res   rslt    = PNR_OK;

    PUBNUB_ASSERT_OPT(user_id != NULL);
    PUBNUB_ASSERT_OPT(message != NULL);

    pb->http_content_len = 0;
    pb->http_buf_len     = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "/publish/%s/%s/0/",
                                pb->publish_key,
                                pb->subscribe_key);
    APPEND_URL_ENCODED_M(pb, channel);
    APPEND_URL_LITERAL_M(pb, "/0");

#if PUBNUB_CRYPTO_API
    if (NULL != pb->crypto_module) {
        pubnub_bymebl_t message_block;
        message_block.ptr  = (uint8_t*)message;
        message_block.size = strlen(message);

        pubnub_bymebl_t encrypted =
            pb->crypto_module->encrypt(pb->crypto_module, message_block);

        if (NULL == encrypted.ptr) {
            PUBNUB_LOG_ERROR("pbcc_publish_prep(pbcc=%p) - encryption failed\n", pb);
            free(message_block.ptr);

            return PNR_INTERNAL_ERROR;
        }

        message = pbcc_base64_encode(encrypted);

        free(encrypted.ptr);

        if (NULL == message) {
            PUBNUB_LOG_ERROR(
                "pbcc_publish_prep(pbcc=%p) - base64 encoding failed\n", pb);
            return PNR_INTERNAL_ERROR;
        }

        char* quoted_message =
            (char*)malloc(strlen(message) + 3); // quotes + null-terminator
        if (NULL == quoted_message) {
            PUBNUB_LOG_ERROR("pbcc_publish_prep(pbcc=%p) - failed to allocate "
                             "memory for quoted message\n",
                             pb);
            free((void*)message);
            return PNR_OUT_OF_MEMORY;
        }

        snprintf(quoted_message, strlen(message) + 3, "\"%s\"", message);
        free((void*)message);
        message = quoted_message;
    }
#endif // PUBNUB_CRYPTO_API

    if (pubnubSendViaGET == method) {
        pb->http_buf[pb->http_buf_len++] = '/';
        APPEND_URL_ENCODED_M(pb, message);
    }
    URL_PARAMS_INIT(qparam, PUBNUB_MAX_URL_PARAMS);
    if (uname) { ADD_URL_PARAM(qparam, pnsdk, uname); }
    ADD_URL_PARAM(qparam, uuid, user_id);
    if (ttl != 0 && ttl != SIZE_MAX) { ADD_URL_PARAM_SIZET(qparam, ttl, ttl); }
#if PUBNUB_CRYPTO_API
    if (pb->secret_key == NULL) { ADD_URL_AUTH_PARAM(pb, qparam, auth); }
    ADD_TS_TO_URL_PARAM();
#else
    ADD_URL_AUTH_PARAM(pb, qparam, auth);
#endif
    if (!store_in_history) { ADD_URL_PARAM(qparam, store, "0"); }
    if (norep) { ADD_URL_PARAM(qparam, norep, "true"); }
    if (meta) { ADD_URL_PARAM(qparam, meta, meta); }
    if (custom_message_type) {
        ADD_URL_PARAM(qparam, custom_message_type, custom_message_type);
    }

#if PUBNUB_CRYPTO_API
    SORT_URL_PARAMETERS(qparam);
#endif
    ENCODE_URL_PARAMETERS(pb, qparam);
#if PUBNUB_CRYPTO_API
    if (pb->secret_key != NULL) {
        rslt = pbcc_sign_url(pb, message, method, false);
        if (rslt != PNR_OK) {
            PUBNUB_LOG_ERROR(
                "pbcc_sign_url failed. pb=%p, message=%s, method=%d", pb, message, method);
        }
    }
#endif

    if (method != pubnubSendViaGET) {
        APPEND_MESSAGE_BODY_M(rslt, pb, message);
    }
    PUBNUB_LOG_DEBUG("pbcc_publish_prep. REQUEST =%s\n", pb->http_buf);
#if PUBNUB_CRYPTO_API
    if (NULL != pb->crypto_module) { free((void*)message); }
#endif

    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}

enum pubnub_res pbcc_sign_url(struct pbcc_context* pc,
                              const char*          msg,
                              enum pubnub_method   method,
                              bool                 v3sign)
{
    enum pubnub_res rslt_         = PNR_INTERNAL_ERROR;
    char*           url           = (char*)malloc(sizeof(pc->http_buf));
    char*           ques          = strchr(pc->http_buf, '?');
    size_t          pos_ques_char = (int)(ques - pc->http_buf);
    if (url != NULL) {
        url[pos_ques_char] = '\0';
        memcpy(url, pc->http_buf, pos_ques_char);
        char final_signature[60];
#if PUBNUB_CRYPTO_API
        char* query_str = ques + 1;
        if (v3sign) {
            rslt_ = pn_gen_pam_v3_sign((pubnub_t *)pc, query_str, url, msg, final_signature, sizeof(final_signature));
        }
        else {
            rslt_ = pn_gen_pam_v2_sign((pubnub_t *)pc, query_str, url, final_signature, sizeof(final_signature));
        }
#endif
        if (rslt_ == PNR_OK) {
            const char* param_ = "signature";
            rslt_              = pbcc_append_url_param(
                (pc),
                param_,
                strlen(param_),
                (final_signature),
                ('&'));
            if (rslt_ != PNR_OK) {
                return rslt_;
            }
        }
    }
    free(url);
    PUBNUB_LOG_DEBUG("\nREQUEST = %s\n", pc->http_buf);
    return rslt_;
}


enum pubnub_res pbcc_signal_prep(struct pbcc_context* pb,
                                 const char*          channel,
                                 const char*          message,
                                 const char*          custom_message_type)
{
    enum pubnub_res   rslt    = PNR_OK;
    char const* const uname   = pubnub_uname();
    char const*       user_id = pbcc_user_id_get(pb);

    PUBNUB_ASSERT_OPT(user_id != NULL);
    PUBNUB_ASSERT_OPT(message != NULL);

    pb->http_content_len = 0;
    pb->http_buf_len     = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "/signal/%s/%s/0/",
                                pb->publish_key,
                                pb->subscribe_key);
    APPEND_URL_ENCODED_M(pb, channel);
    APPEND_URL_LITERAL_M(pb, "/0/");
    APPEND_URL_ENCODED_M(pb, message);

    URL_PARAMS_INIT(qparam, PUBNUB_MAX_URL_PARAMS);
    if (uname) { ADD_URL_PARAM(qparam, pnsdk, uname); }
    if (user_id) { ADD_URL_PARAM(qparam, uuid, user_id); }
    if (custom_message_type) {
        ADD_URL_PARAM(qparam, custom_message_type, custom_message_type);
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
        if (rslt != PNR_OK) {
            PUBNUB_LOG_ERROR("pbcc_sign_url failed. pb=%p, message=%s, method=%d", pb, "", pubnubSendViaGET);
        }
    }
#endif
    PUBNUB_LOG_DEBUG("pbcc_signal_prep. REQUEST =%s\n", pb->http_buf);
    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_subscribe_prep(struct pbcc_context* p,
                                    char const*          channel,
                                    char const*          channel_group,
                                    const unsigned*      heartbeat)
{
    char const*       user_id = pbcc_user_id_get(p);
    char const* const uname   = pubnub_uname();

    PUBNUB_ASSERT_OPT(user_id != NULL);

    enum pubnub_res rslt = PNR_OK;

    if (NULL == channel) {
        if (NULL == channel_group) {
            return PNR_INVALID_CHANNEL;
        }
        channel = ",";
    }
    if (p->msg_ofs < p->msg_end) {
        return PNR_RX_BUFF_NOT_EMPTY;
    }

    p->http_content_len = 0;
    p->msg_ofs          = p->msg_end = 0;

#if PUBNUB_CRYPTO_API
    for (size_t i = 0; i < p->decrypted_message_count; i++) {
        free(p->decrypted_messages[i]);
    }
    p->decrypted_message_count = 0;
#endif

    p->http_buf_len = snprintf(
        p->http_buf,
        sizeof p->http_buf,
        "/subscribe/%s/",
        p->subscribe_key);
    APPEND_URL_ENCODED_M(p, channel);
    p->http_buf_len += snprintf(p->http_buf + p->http_buf_len,
                                sizeof p->http_buf - p->http_buf_len,
                                "/0/%s",
                                p->timetoken);
    URL_PARAMS_INIT(qparam, PUBNUB_MAX_URL_PARAMS);
    if (uname) { ADD_URL_PARAM(qparam, pnsdk, uname); }
    if (channel_group) { ADD_URL_PARAM(qparam, channel-group, channel_group); }
    if (user_id) { ADD_URL_PARAM(qparam, uuid, user_id); }
#if PUBNUB_CRYPTO_API
    if (p->secret_key == NULL) { ADD_URL_AUTH_PARAM(p, qparam, auth); }
    ADD_TS_TO_URL_PARAM();
#else
    ADD_URL_AUTH_PARAM(p, qparam, auth);
#endif

    if (heartbeat) {
        ADD_URL_PARAM_SIZET(qparam, heartbeat, (unsigned long)(*heartbeat));
    }

#if PUBNUB_CRYPTO_API
  SORT_URL_PARAMETERS(qparam);
#endif
    ENCODE_URL_PARAMETERS(p, qparam);
#if PUBNUB_CRYPTO_API
    if (p->secret_key != NULL) {
        rslt = pbcc_sign_url(p, "", pubnubSendViaGET, true);
    }
#endif
    PUBNUB_LOG_DEBUG("pbcc_subscribe_prep. REQUEST =%s\n", p->http_buf);
    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}
