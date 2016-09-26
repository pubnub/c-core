/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_ccore.h"
#include "pubnub_version.h"
#include "pubnub_assert.h"
#include "pubnub_internal.h"
#include "pubnub_json_parse.h"
#include "pubnub_log.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>


void pbcc_init(struct pbcc_context *p, const char *publish_key, const char *subscribe_key)
{
    p->publish_key = publish_key;
    p->subscribe_key = subscribe_key;
    p->timetoken[0] = '0';
    p->timetoken[1] = '\0';
    p->uuid = p->auth = NULL;
    p->msg_ofs = p->msg_end = 0;
    if (PUBNUB_DYNAMIC_REPLY_BUFFER) {
        p->http_reply = NULL;
    }

#if PUBNUB_CRYPTO_API
    p->secret_key = NULL;
#endif
}


void pbcc_deinit(struct pbcc_context *p)
{
    if (PUBNUB_DYNAMIC_REPLY_BUFFER) {
        if (p->http_reply != NULL) {
            free(p->http_reply);
            p->http_reply = NULL;
        }
    }
}


int pbcc_realloc_reply_buffer(struct pbcc_context *p, unsigned bytes)
{
#if PUBNUB_DYNAMIC_REPLY_BUFFER
    char *newbuf = (char*)realloc(p->http_reply, bytes + 1);
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


char const *pbcc_get_msg(struct pbcc_context *pb)
{
    if (pb->msg_ofs < pb->msg_end) {
        char const *rslt = pb->http_reply + pb->msg_ofs;
        pb->msg_ofs += strlen(rslt);
        if (pb->msg_ofs++ <= pb->msg_end) {
            return rslt;
        }
    }

    return NULL;
}


char const *pbcc_get_channel(struct pbcc_context *pb)
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


void pbcc_set_uuid(struct pbcc_context *pb, const char *uuid)
{
    pb->uuid = uuid;
}


void pbcc_set_auth(struct pbcc_context *pb, const char *auth)
{
    pb->auth = auth;
}


/* Find the beginning of a JSON string that comes after comma and ends
 * at @c &buf[len].
 * @return position (index) of the found start or -1 on error. */
static int find_string_start(char const *buf, int len)
{
    int i;
    for (i = len-1; i > 0; --i) {
        if (buf[i] == '"') {
            return (buf[i-1] == ',') ? i : -1;
        }
    }
    return -1;
}


/** Split @p buf string containing a JSON array (with arbitrary
 * contents) to multiple NUL-terminated C strings, in-place.
 */
static bool split_array(char *buf)
{
    bool escaped = false;
    bool in_string = false;
    int bracket_level = 0;

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
            case '[': case '{': bracket_level++; break;
            case ']': case '}': bracket_level--; break;
                /* if at root, split! */
            case ',': if (bracket_level == 0) { *buf = '\0'; } break;
            default: break;
            }
        }
    }

    PUBNUB_LOG_TRACE("escaped = %d, in_string = %d, bracket_level = %d\n", escaped, in_string, bracket_level);
    return !(escaped || in_string || (bracket_level > 0));
}


static int simple_parse_response(struct pbcc_context *p)
{
    char *reply = p->http_reply;
    int replylen = p->http_buf_len;
    if (replylen < 2) {
        return -1;
    }
    if ((reply[0] != '[') || (reply[replylen-1] != ']')) {
        return -1;
    }

    p->chan_ofs = 0;
    p->chan_end = 0;

    p->msg_ofs = 1;
    p->msg_end = replylen - 1;
    reply[replylen-1] = '\0';

    return split_array(reply + p->msg_ofs) ? 0 : -1;
}


enum pubnub_res pbcc_parse_publish_response(struct pbcc_context *p)
{
    char *reply = p->http_reply;
    int replylen = p->http_buf_len;
    if (replylen < 2) {
        return PNR_FORMAT_ERROR;
    }
    if ((reply[0] != '[') || (reply[replylen-1] != ']')) {
        return PNR_FORMAT_ERROR;
    }

    p->chan_ofs = p->chan_end = 0;
    p->msg_ofs = p->msg_end = 0;

    reply[replylen-1] = '\0';

    if (split_array(reply + 1)) {
        if (1 != strtol(reply+1, NULL, 10)) {
            return PNR_PUBLISH_FAILED;
        }
        return PNR_OK;
    }
    else {
        return PNR_FORMAT_ERROR;
    }
}

int pbcc_parse_time_response(struct pbcc_context *p)
{
    return simple_parse_response(p);
}


int pbcc_parse_history_response(struct pbcc_context *p)
{
    return simple_parse_response(p);
}


int pbcc_parse_presence_response(struct pbcc_context *p)
{
    char *reply = p->http_reply;
    int replylen = p->http_buf_len;
    if (replylen < 2) {
        return -1;
    }
    if ((reply[0] != '{') || (reply[replylen-1] != '}')) {
        return -1;
    }

    p->chan_ofs = p->chan_end = 0;

    p->msg_ofs = 0;
    p->msg_end = replylen;

    return 0;
}


enum pubnub_res pbcc_parse_channel_registry_response(struct pbcc_context *p)
{
    enum pbjson_object_name_parse_result result;
    struct pbjson_elem el;
    struct pbjson_elem found;

	el.start = p->http_reply;
	el.end = p->http_reply + p->http_buf_len;
    p->chan_ofs = 0;
    p->chan_end = p->http_buf_len;

    p->msg_ofs = p->msg_end = 0;

    /* We should probably also check that there is a key "service"
       with value "channel-registry".  Maybe even that there is a key
       "status" (with value 200).
    */
    result = pbjson_get_object_value(&el, "error", &found);
    if (jonmpOK == result) {
        if (pbjson_elem_equals_string(&found, "false")) {
            return PNR_OK;
        }
        else {
            return PNR_CHANNEL_REGISTRY_ERROR;
        }
    }
    else {
        return PNR_FORMAT_ERROR;
    }
}


int pbcc_parse_subscribe_response(struct pbcc_context *p)
{
    int i;
	int previous_i;
	unsigned time_token_length;
    char *reply = p->http_reply;
    int replylen = p->http_buf_len;
    if (replylen < 2) {
        return -1;
    }
    if (reply[replylen-1] != ']' && replylen > 2) {
        replylen -= 2; /* XXX: this seems required by Manxiang */
    }
    if ((reply[0] != '[') || (reply[replylen-1] != ']') || (reply[replylen-2] != '"')) {
        return -1;
    }

    /* Extract the last argument. */
	previous_i = replylen - 2;
    i = find_string_start(reply, previous_i);
    if (i < 0) {
        return -1;
    }
    reply[replylen - 2] = 0;

    /* Now, the last argument may either be a timetoken, a channel group list
		or a channel list. */
    if (reply[i-2] == '"') {
        int k;
		/* It is a channel list, there is another string argument in front
         * of us. Process the channel list ... */
        for (k = replylen - 2; k > i+1; --k) {
            if (reply[k] == ',') {
                reply[k] = '\0';
            }
        }

        /* The previous argument is either a timetoken or a channel group
			list. */
        reply[i-2] = '\0';
        p->chan_ofs = i+1;
        p->chan_end = replylen - 1;
		previous_i = i-2;
        i = find_string_start(reply, previous_i);
        if (i < 0) {
            p->chan_ofs = 0;
            p->chan_end = 0;
            return -1;
        }
		if (reply[i-2] == '"') {
			/* It is a channel group list. For now, we shall skip it. In
				the future, we may process it like we do the channel list.
				*/
			reply[i-2] = '\0';
			previous_i = i-2;
			i = find_string_start(reply, previous_i);
			if (i < 0) {
				return -1;
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
	time_token_length = previous_i - (i+1);
	if (time_token_length >= sizeof p->timetoken) {
		p->timetoken[0] = '\0';
		return -1;
	}
	memcpy(p->timetoken, reply + i+1, time_token_length+1);
	
	/* terminate the [] message array (before the `]`!) */
    reply[i-2] = 0; 

    /* Set up the message list - offset, length and NUL-characters
     * splitting the messages. */
    p->msg_ofs = 2;
    p->msg_end = i-2;

    return split_array(reply + p->msg_ofs) ? 0 : -1;
}


static enum pubnub_res append_url_param(struct pbcc_context *pb, char const *param_name, size_t param_name_len, char const *param_val, char separator)
{
    size_t param_val_len = strlen(param_val);
    if (pb->http_buf_len + 1 + param_name_len + 1 + param_val_len > sizeof pb->http_buf) {
        return PNR_TX_BUFF_TOO_SMALL;
    }

    pb->http_buf[pb->http_buf_len++] = separator;
    memcpy(pb->http_buf + pb->http_buf_len, param_name, param_name_len);
    pb->http_buf_len += param_name_len;
    pb->http_buf[pb->http_buf_len++] = '=';
    memcpy(pb->http_buf + pb->http_buf_len, param_val, param_val_len+1);
    pb->http_buf_len += param_val_len;

    return PNR_OK;
}


#define APPEND_URL_PARAM_M(pbc, name, var, separator)                   \
    if ((var) != NULL) {                                                \
        const char param_[] = name;                                 	\
        enum pubnub_res rslt_ = append_url_param((pbc), param_, sizeof param_ - 1, (var), (separator)); \
        if (rslt_ != PNR_OK) {                                          \
            return rslt_;                                               \
        }                                                               \
    }


#define APPEND_URL_PARAM_INT_M(pbc, name, var, separator)               \
    do { char v_[20];                                                   \
        snprintf(v_, sizeof v_, "%d", (var));                           \
        APPEND_URL_PARAM_M(pbc, name, v_, separator);                   \
    } while (0)


#define APPEND_URL_PARAM_UNSIGNED_M(pbc, name, var, separator)          \
    do { char v_[20];                                                   \
        snprintf(v_, sizeof v_, "%u", (var));                           \
        APPEND_URL_PARAM_M(pbc, name, v_, separator);                   \
    } while (0)

#define APPEND_URL_OPT_PARAM_UNSIGNED_M(pbc, name, var, separator)      \
    if ((var) != NULL) { char v_[20];                                   \
        snprintf(v_, sizeof v_, "%u", *(var));                          \
        APPEND_URL_PARAM_M(pbc, name, v_, separator);                   \
    } while (0)


#define APPEND_URL_PARAM_BOOL_M(pbc, name, var, separator)              \
    do { char const *v_ = (var) ? "true" : "false";                     \
        APPEND_URL_PARAM_M(pbc, name, v_, separator);                   \
    } while (0)

#define APPEND_URL_PARAM_TRIBOOL_M(pbc, name, var, separator)           \
    if ((var) != pbccNotSet) {                                          \
        char const *v_ = (var) ? "1" : "0";                             \
        APPEND_URL_PARAM_M(pbc, name, v_, separator);                   \
    }


enum pubnub_res pbcc_publish_prep(struct pbcc_context *pb, const char *channel, const char *message, bool store_in_history, bool eat_after_reading)
{
    char const *const uname = pubnub_uname();
    char const *pmessage = message;
    pb->http_content_len = 0;

    pb->http_buf_len = snprintf(
        pb->http_buf, sizeof pb->http_buf,
        "/publish/%s/%s/0/%s/0/",
        pb->publish_key, pb->subscribe_key, channel
        );


    while (pmessage[0]) {
        /* RFC 3986 Unreserved characters plus few
         * safe reserved ones. */
        size_t okspan = strspn(pmessage, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.~" ",=:;@[]");
        if (okspan > 0) {
            if (okspan > sizeof(pb->http_buf)-1 - pb->http_buf_len) {
                pb->http_buf_len = 0;
                return PNR_TX_BUFF_TOO_SMALL;
            }
            memcpy(pb->http_buf + pb->http_buf_len, pmessage, okspan);
            pb->http_buf_len += okspan;
            pb->http_buf[pb->http_buf_len] = 0;
            pmessage += okspan;
        }
        if (pmessage[0]) {
            /* %-encode a non-ok character. */
            char enc[4] = {'%'};
            enc[1] = "0123456789ABCDEF"[pmessage[0] / 16];
            enc[2] = "0123456789ABCDEF"[pmessage[0] % 16];
            if (3 > sizeof pb->http_buf - 1 - pb->http_buf_len) {
                pb->http_buf_len = 0;
                return PNR_TX_BUFF_TOO_SMALL;
            }
            memcpy(pb->http_buf + pb->http_buf_len, enc, 4);
            pb->http_buf_len += 3;
            ++pmessage;
        }
    }
    APPEND_URL_PARAM_M(pb, "pnsdk", uname, '?');
    APPEND_URL_PARAM_M(pb, "uuid", pb->uuid, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');
    if (!store_in_history) {
        APPEND_URL_PARAM_BOOL_M(pb, "store", store_in_history, '?');
    }
    if (eat_after_reading) {
        APPEND_URL_PARAM_BOOL_M(pb, "ear", eat_after_reading, '?');
    }

    return PNR_STARTED;
}


enum pubnub_res pbcc_subscribe_prep(struct pbcc_context *p, const char *channel, const char *channel_group, unsigned *heartbeat)
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

    p->http_buf_len = snprintf(
        p->http_buf, sizeof(p->http_buf),
        "/subscribe/%s/%s/0/%s?pnsdk=%s",
        p->subscribe_key, channel, p->timetoken,
        pubnub_uname()
        );
    APPEND_URL_PARAM_M(p, "channel-group", channel_group, '&');
    APPEND_URL_PARAM_M(p, "uuid", p->uuid, '&');
    APPEND_URL_PARAM_M(p, "auth", p->auth, '&');
    APPEND_URL_OPT_PARAM_UNSIGNED_M(p, "heartbeat", heartbeat, '&');

    return PNR_STARTED;
}


enum pubnub_res pbcc_leave_prep(struct pbcc_context *pb, const char *channel, const char *channel_group)
{
    if (NULL == channel) {
        if (NULL == channel_group) {
            return PNR_INVALID_CHANNEL;
        }
        channel = ",";
    }
    pb->http_content_len = 0;

    /* Make sure next subscribe() will be a join. */
    pb->timetoken[0] = '0';
    pb->timetoken[1] = '\0';

    pb->http_buf_len = snprintf(
        pb->http_buf, sizeof pb->http_buf,
        "/v2/presence/sub-key/%s/channel/%s/leave?pnsdk=%s",
        pb->subscribe_key, channel,
        pubnub_uname()
        );
    APPEND_URL_PARAM_M(pb, "channel-group", channel_group, '&');
    APPEND_URL_PARAM_M(pb, "uuid", pb->uuid, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');

    return PNR_STARTED;
}


enum pubnub_res pbcc_time_prep(struct pbcc_context *pb)
{
    if (pb->msg_ofs < pb->msg_end) {
        return PNR_RX_BUFF_NOT_EMPTY;
    }

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;

    pb->http_buf_len = snprintf(
        pb->http_buf, sizeof pb->http_buf,
        "/time/0?pnsdk=%s",
        pubnub_uname()
        );
    APPEND_URL_PARAM_M(pb, "uuid", pb->uuid, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');

    return PNR_STARTED;
}



enum pubnub_res pbcc_history_prep(struct pbcc_context *pb, const char *channel, unsigned count, bool include_token)
{
    if (pb->msg_ofs < pb->msg_end) {
        return PNR_RX_BUFF_NOT_EMPTY;
    }

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;

    pb->http_buf_len = snprintf(
        pb->http_buf, sizeof pb->http_buf,
        "/v2/history/sub-key/%s/channel/%s?pnsdk=%s",
        pb->subscribe_key, channel,
        pubnub_uname()
        );
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');
    APPEND_URL_PARAM_UNSIGNED_M(pb, "count", count, '&');
    APPEND_URL_PARAM_BOOL_M(pb, "include_token", include_token, '&');

    return PNR_STARTED;
}


enum pubnub_res pbcc_heartbeat_prep(struct pbcc_context *pb, const char *channel, const char *channel_group)
{
    if (NULL == channel) {
        if (NULL == channel_group) {
            return PNR_INVALID_CHANNEL;
        }
        channel = ",";
    }
    if (pb->msg_ofs < pb->msg_end) {
        return PNR_RX_BUFF_NOT_EMPTY;
    }

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;

    pb->http_buf_len = snprintf(
        pb->http_buf, sizeof pb->http_buf,
        "/v2/presence/sub-key/%s/channel/%s/heartbeat?pnsdk=%s",
        pb->subscribe_key,
        channel,
        pubnub_uname()
        );
    APPEND_URL_PARAM_M(pb, "channel-group", channel_group, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');
    APPEND_URL_PARAM_M(pb, "uuid", pb->uuid, '&');

    return PNR_STARTED;
}


enum pubnub_res pbcc_here_now_prep(struct pbcc_context *pb, const char *channel, const char *channel_group, enum pbcc_tribool disable_uuids, enum pbcc_tribool state)
{
    if (NULL == channel) {
        if (channel_group != NULL) {
            channel = ",";
        }
    }
    if (pb->msg_ofs < pb->msg_end) {
        return PNR_RX_BUFF_NOT_EMPTY;
    }

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;

    pb->http_buf_len = snprintf(
        pb->http_buf, sizeof pb->http_buf,
        "/v2/presence/sub-key/%s%s%s?pnsdk=%s",
        pb->subscribe_key,
        channel ? "/channel/" : "",
        channel ? channel : "",
        pubnub_uname()
        );
    APPEND_URL_PARAM_M(pb, "channel-group", channel_group, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');
    APPEND_URL_PARAM_M(pb, "uuid", pb->uuid, '&');
    APPEND_URL_PARAM_TRIBOOL_M(pb, "disable_uuids", disable_uuids, '&');
    APPEND_URL_PARAM_TRIBOOL_M(pb, "state", state, '&');

    return PNR_STARTED;
}


enum pubnub_res pbcc_where_now_prep(struct pbcc_context *pb, const char *uuid)
{
    PUBNUB_ASSERT_OPT(uuid != NULL);

    if (pb->msg_ofs < pb->msg_end) {
        return PNR_RX_BUFF_NOT_EMPTY;
    }

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;

    pb->http_buf_len = snprintf(
        pb->http_buf, sizeof pb->http_buf,
        "/v2/presence/sub-key/%s/uuid/%s?pnsdk=%s",
        pb->subscribe_key, uuid,
        pubnub_uname()
        );
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');
    return PNR_STARTED;
}


enum pubnub_res pbcc_set_state_prep(struct pbcc_context *pb, char const *channel, char const *channel_group, const char *uuid, char const *state)
{
    PUBNUB_ASSERT_OPT(uuid != NULL);
    PUBNUB_ASSERT_OPT(state != NULL);

    if (NULL == channel) {
        if (NULL == channel_group) {
            return PNR_INVALID_CHANNEL;
        }
        channel = ",";
    }
    if (pb->msg_ofs < pb->msg_end) {
        return PNR_RX_BUFF_NOT_EMPTY;
    }

    pb->http_buf_len = snprintf(
        pb->http_buf, sizeof pb->http_buf,
        "/v2/presence/sub-key/%s/channel/%s/uuid/%s/data?pnsdk=%s&state=%s",
        pb->subscribe_key, channel, uuid,
        pubnub_uname(), state
        );
    APPEND_URL_PARAM_M(pb, "channel-group", channel_group, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');

    return PNR_STARTED;
}


enum pubnub_res pbcc_state_get_prep(struct pbcc_context *pb, char const *channel, char const *channel_group, const char *uuid)
{
    PUBNUB_ASSERT_OPT(uuid != NULL);

    if (NULL == channel) {
        if (NULL == channel_group) {
            return PNR_INVALID_CHANNEL;
        }
        channel = ",";
    }
    if (pb->msg_ofs < pb->msg_end) {
        return PNR_RX_BUFF_NOT_EMPTY;
    }

    pb->http_buf_len = snprintf(
        pb->http_buf, sizeof pb->http_buf,
        "/v2/presence/sub-key/%s/channel/%s/uuid/%s?pnsdk=%s",
        pb->subscribe_key, channel, uuid,
        pubnub_uname()
        );
    APPEND_URL_PARAM_M(pb, "channel-group", channel_group, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');

    return PNR_STARTED;
}


enum pubnub_res pbcc_remove_channel_group_prep(struct pbcc_context *pb, char const *channel_group)
{
    PUBNUB_ASSERT_OPT(channel_group != NULL);

    pb->http_buf_len = snprintf(
        pb->http_buf, sizeof pb->http_buf,
        "/v1/channel-registration/sub-key/%s/channel-group/%s/remove?pnsdk=%s",
        pb->subscribe_key, channel_group, pubnub_uname()
        );
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');

    return PNR_STARTED;
}


enum pubnub_res pbcc_channel_registry_prep(struct pbcc_context *pb, char const *channel_group, char const *param, char const *channel)
{
    PUBNUB_ASSERT_OPT(channel_group != NULL);

    pb->http_buf_len = snprintf(
        pb->http_buf, sizeof pb->http_buf,
        "/v1/channel-registration/sub-key/%s/channel-group/%s?pnsdk=%s",
        pb->subscribe_key, channel_group, pubnub_uname()
        );
    if (NULL != param) {
        enum pubnub_res rslt;
        PUBNUB_ASSERT_OPT(channel != NULL);
        rslt = append_url_param(pb, param, strlen(param), channel, '&');
        if (rslt != PNR_OK) {
            return rslt;
        }
    }
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');

    return PNR_STARTED;
}
