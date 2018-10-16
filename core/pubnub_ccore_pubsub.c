/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_ccore_pubsub.h"
#include "pubnub_version.h"
#include "pubnub_assert.h"
#include "pubnub_internal.h"
#include "pubnub_json_parse.h"
#include "pubnub_log.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>


void pbcc_init(struct pbcc_context* p, const char* publish_key, const char* subscribe_key)
{
    p->publish_key   = publish_key;
    p->subscribe_key = subscribe_key;
    p->timetoken[0]  = '0';
    p->timetoken[1]  = '\0';
    p->uuid = p->auth = NULL;
    p->msg_ofs = p->msg_end = 0;
#if PUBNUB_DYNAMIC_REPLY_BUFFER
    p->http_reply = NULL;
#if PUBNUB_RECEIVE_GZIP_RESPONSE
    p->decomp_buf_size   = (size_t)0;
    p->decomp_http_reply = NULL;
#endif /* PUBNUB_RECEIVE_GZIP_RESPONSE */
#endif /* PUBNUB_DYNAMIC_REPLY_BUFFER */

#if PUBNUB_CRYPTO_API
    p->secret_key = NULL;
#endif
}


void pbcc_deinit(struct pbcc_context* p)
{
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
    if (pb->msg_ofs < pb->msg_end) {
        char const* rslt = pb->http_reply + pb->msg_ofs;
        pb->msg_ofs += strlen(rslt);
        if (pb->msg_ofs++ <= pb->msg_end) {
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


void pbcc_set_uuid(struct pbcc_context* pb, const char* uuid)
{
    pb->uuid = uuid;
}


void pbcc_set_auth(struct pbcc_context* pb, const char* auth)
{
    pb->auth = auth;
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
    char* reply    = p->http_reply;
    int   replylen = p->http_buf_len;
    if (replylen < 2) {
        return PNR_FORMAT_ERROR;
    }

    p->chan_ofs = p->chan_end = 0;
    p->msg_ofs = p->msg_end = 0;

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
    int      i;
    int      previous_i;
    unsigned time_token_length;
    char*    reply    = p->http_reply;
    int      replylen = p->http_buf_len;
    if (replylen < 2) {
        return PNR_FORMAT_ERROR;
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


static enum pubnub_res url_encode(struct pbcc_context* pb, char const* what)
{
    PUBNUB_ASSERT_OPT(pb != NULL);
    PUBNUB_ASSERT_OPT(what != NULL);

    while (what[0]) {
        /* RFC 3986 Unreserved characters plus few
         * safe reserved ones. */
        size_t okspan = strspn(
            what,
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.~"
            ",=:;@[]");
        if (okspan > 0) {
            if (okspan > sizeof(pb->http_buf) - 1 - pb->http_buf_len) {
                pb->http_buf_len = 0;
                return PNR_TX_BUFF_TOO_SMALL;
            }
            memcpy(pb->http_buf + pb->http_buf_len, what, okspan);
            pb->http_buf_len += okspan;
            pb->http_buf[pb->http_buf_len] = 0;
            what += okspan;
        }
        if (what[0]) {
            /* %-encode a non-ok character. */
            char enc[4] = { '%' };
            enc[1]      = "0123456789ABCDEF"[(unsigned char)what[0] / 16];
            enc[2]      = "0123456789ABCDEF"[(unsigned char)what[0] % 16];
            if (3 > sizeof pb->http_buf - 1 - pb->http_buf_len) {
                pb->http_buf_len = 0;
                return PNR_TX_BUFF_TOO_SMALL;
            }
            memcpy(pb->http_buf + pb->http_buf_len, enc, 4);
            pb->http_buf_len += 3;
            ++what;
        }
    }

    return PNR_OK;
}


enum pubnub_res pbcc_append_url_param_encoded(struct pbcc_context* pb,
                                      char const*          param_name,
                                      size_t               param_name_len,
                                      char const*          param_val,
                                      char                 separator)
{
    if (pb->http_buf_len + 1 + param_name_len + 1 > sizeof pb->http_buf) {
        return PNR_TX_BUFF_TOO_SMALL;
    }

    pb->http_buf[pb->http_buf_len++] = separator;
    memcpy(pb->http_buf + pb->http_buf_len, param_name, param_name_len);
    pb->http_buf_len += param_name_len;
    pb->http_buf[pb->http_buf_len++] = '=';
    return url_encode(pb, param_val);
}


enum pubnub_res pbcc_publish_prep(struct pbcc_context* pb,
                                  const char*          channel,
                                  const char*          message,
                                  bool                 store_in_history,
                                  bool                 norep,
                                  char const*          meta)
{
    char const* const uname = pubnub_uname();
    enum pubnub_res   rslt;

    PUBNUB_ASSERT_OPT(message != NULL);

    pb->http_content_len = 0;
    pb->http_buf_len     = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "/publish/%s/%s/0/%s/0/",
                                pb->publish_key,
                                pb->subscribe_key,
                                channel);

    rslt = url_encode(pb, message);
    if (rslt != PNR_OK) {
        return rslt;
    }
    APPEND_URL_PARAM_M(pb, "pnsdk", uname, '?');
    APPEND_URL_PARAM_M(pb, "uuid", pb->uuid, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');
    if ((PNR_OK == rslt) && !store_in_history) {
        rslt = pbcc_append_url_param(pb, "store", sizeof "store" - 1, "0", '&');
    }
    if ((PNR_OK == rslt) && norep) {
        rslt = pbcc_append_url_param(pb, "norep", sizeof "norep" - 1, "true", '&');
    }
    if ((PNR_OK == rslt) && (meta != NULL)) {
        size_t const param_name_len = sizeof "meta" - 1;
        if (pb->http_buf_len + 1 + param_name_len + 1 + 1 > sizeof pb->http_buf) {
            return PNR_TX_BUFF_TOO_SMALL;
        }
        pb->http_buf[pb->http_buf_len++] = '&';
        memcpy(pb->http_buf + pb->http_buf_len, "meta", param_name_len);
        pb->http_buf_len += param_name_len;
        pb->http_buf[pb->http_buf_len++] = '=';
        rslt                             = url_encode(pb, meta);
    }

    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_subscribe_prep(struct pbcc_context* p,
                                    char const*          channel,
                                    char const*          channel_group,
                                    unsigned*            heartbeat)
{
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
    p->msg_ofs = p->msg_end = 0;

    p->http_buf_len = snprintf(p->http_buf,
                               sizeof(p->http_buf),
                               "/subscribe/%s/%s/0/%s?pnsdk=%s",
                               p->subscribe_key,
                               channel,
                               p->timetoken,
                               pubnub_uname());
    APPEND_URL_PARAM_M(p, "channel-group", channel_group, '&');
    APPEND_URL_PARAM_M(p, "uuid", p->uuid, '&');
    APPEND_URL_PARAM_M(p, "auth", p->auth, '&');
    APPEND_URL_OPT_PARAM_UNSIGNED_M(p, "heartbeat", heartbeat, '&');

    return PNR_STARTED;
}
