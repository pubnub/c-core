/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_ccore.h"
#include "pubnub_ccore_pubsub.h"
#include "pubnub_version.h"
#include "pubnub_assert.h"
#include "pubnub_internal.h"
#include "pubnub_json_parse.h"
#include "pubnub_log.h"

#include <stdio.h>


static enum pubnub_res simple_parse_response(struct pbcc_context* p)
{
    char* reply    = p->http_reply;
    int   replylen = p->http_buf_len;
    if (replylen < 2) {
        return PNR_FORMAT_ERROR;
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
    char* reply    = p->http_reply;
    int   replylen = p->http_buf_len;
    if (replylen < 2) {
        return PNR_FORMAT_ERROR;
    }
    if ((reply[0] != '{') || (reply[replylen - 1] != '}')) {
        return PNR_FORMAT_ERROR;
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

    el.start    = p->http_reply;
    el.end      = p->http_reply + p->http_buf_len;
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


enum pubnub_res pbcc_leave_prep(struct pbcc_context* pb,
                                const char*          channel,
                                const char*          channel_group)
{
    char const* uuid = pbcc_uuid_get(pb);

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

    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "/v2/presence/sub-key/%s/channel/",
                                pb->subscribe_key);
    APPEND_URL_ENCODED_M(pb, channel);
    pb->http_buf_len += snprintf(pb->http_buf + pb->http_buf_len,
                                 sizeof pb->http_buf - pb->http_buf_len,
                                 "/leave?pnsdk=%s",
                                 pubnub_uname());
    APPEND_URL_PARAM_M(pb, "channel-group", channel_group, '&');
    APPEND_URL_PARAM_M(pb, "uuid", uuid, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');

    return PNR_STARTED;
}


enum pubnub_res pbcc_time_prep(struct pbcc_context* pb)
{
    char const* uuid = pbcc_uuid_get(pb);

    if (pb->msg_ofs < pb->msg_end) {
        return PNR_RX_BUFF_NOT_EMPTY;
    }

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;

    pb->http_buf_len = snprintf(
        pb->http_buf, sizeof pb->http_buf, "/time/0?pnsdk=%s", pubnub_uname());
    APPEND_URL_PARAM_M(pb, "uuid", uuid, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');

    return PNR_STARTED;
}


enum pubnub_res pbcc_history_prep(struct pbcc_context* pb,
                                  const char*          channel,
                                  unsigned             count,
                                  bool                 include_token,
                                  enum pubnub_tribool  string_token,
                                  enum pubnub_tribool  reverse,
                                  char const*          start,
                                  char const*          end)
{
    char const* const uname = pubnub_uname();

    if (pb->msg_ofs < pb->msg_end) {
        return PNR_RX_BUFF_NOT_EMPTY;
    }

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;

    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "/v2/history/sub-key/%s/channel/",
                                pb->subscribe_key);
    APPEND_URL_ENCODED_M(pb, channel);
    APPEND_URL_PARAM_M(pb, "pnsdk", uname, '?');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');
    APPEND_URL_PARAM_UNSIGNED_M(pb, "count", count, '&');
    APPEND_URL_PARAM_BOOL_M(pb, "include_token", include_token, '&');
    APPEND_URL_PARAM_TRIBOOL_M(pb, "stringtoken", string_token, '&');
    APPEND_URL_PARAM_TRIBOOL_M(pb, "reverse", reverse, '&');
    APPEND_URL_PARAM_M(pb, "start", start, '&');
    APPEND_URL_PARAM_M(pb, "start", end, '&');

    return PNR_STARTED;
}


enum pubnub_res pbcc_heartbeat_prep(struct pbcc_context* pb,
                                    const char*          channel,
                                    const char*          channel_group)
{
    char const* uuid = pbcc_uuid_get(pb);

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

    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "/v2/presence/sub-key/%s/channel/",
                                pb->subscribe_key);
    APPEND_URL_ENCODED_M(pb, channel);
    pb->http_buf_len  += snprintf(pb->http_buf + pb->http_buf_len,
                                  sizeof pb->http_buf - pb->http_buf_len,
                                  "/heartbeat?pnsdk=%s",
                                  pubnub_uname());
    APPEND_URL_PARAM_M(pb, "channel-group", channel_group, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');
    APPEND_URL_PARAM_M(pb, "uuid", uuid, '&');

    return PNR_STARTED;
}


enum pubnub_res pbcc_here_now_prep(struct pbcc_context* pb,
                                   const char*          channel,
                                   const char*          channel_group,
                                   enum pubnub_tribool  disable_uuids,
                                   enum pubnub_tribool  state)
{
    char const* const uname = pubnub_uname();
    char const*       uuid  = pbcc_uuid_get(pb);

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

    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "/v2/presence/sub-key/%s%s",
                                pb->subscribe_key,
                                channel ? "/channel/" : "");
    APPEND_URL_ENCODED_M(pb, channel);
    APPEND_URL_PARAM_M(pb, "pnsdk", uname, '?');
    APPEND_URL_PARAM_M(pb, "channel-group", channel_group, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');
    APPEND_URL_PARAM_M(pb, "uuid", uuid, '&');
    APPEND_URL_PARAM_TRIBOOL_M(pb, "disable_uuids", disable_uuids, '&');
    APPEND_URL_PARAM_TRIBOOL_M(pb, "state", state, '&');

    return PNR_STARTED;
}


enum pubnub_res pbcc_where_now_prep(struct pbcc_context* pb, const char* uuid)
{
    PUBNUB_ASSERT_OPT(uuid != NULL);

    if (pb->msg_ofs < pb->msg_end) {
        return PNR_RX_BUFF_NOT_EMPTY;
    }

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;

    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "/v2/presence/sub-key/%s/uuid/%s?pnsdk=%s",
                                pb->subscribe_key,
                                uuid,
                                pubnub_uname());
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');
    return PNR_STARTED;
}


enum pubnub_res pbcc_set_state_prep(struct pbcc_context* pb,
                                    char const*          channel,
                                    char const*          channel_group,
                                    const char*          uuid,
                                    char const*          state)
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

    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "/v2/presence/sub-key/%s/channel/",
                                pb->subscribe_key);
    APPEND_URL_ENCODED_M(pb, channel);
    pb->http_buf_len += snprintf(pb->http_buf + pb->http_buf_len,
                                 sizeof pb->http_buf - pb->http_buf_len,
                                 "/uuid/%s/data?pnsdk=%s&state=%s",
                                 uuid,
                                 pubnub_uname(),
                                 state);
    APPEND_URL_PARAM_M(pb, "channel-group", channel_group, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');

    return PNR_STARTED;
}


enum pubnub_res pbcc_state_get_prep(struct pbcc_context* pb,
                                    char const*          channel,
                                    char const*          channel_group,
                                    const char*          uuid)
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

    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "/v2/presence/sub-key/%s/channel/",
                                pb->subscribe_key);
    APPEND_URL_ENCODED_M(pb, channel);
    pb->http_buf_len += snprintf(pb->http_buf + pb->http_buf_len,
                                 sizeof pb->http_buf - pb->http_buf_len,
                                 "/uuid/%s?pnsdk=%s",
                                 uuid,
                                 pubnub_uname());
    APPEND_URL_PARAM_M(pb, "channel-group", channel_group, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');

    return PNR_STARTED;
}


enum pubnub_res pbcc_remove_channel_group_prep(struct pbcc_context* pb,
                                               char const* channel_group)
{
    PUBNUB_ASSERT_OPT(channel_group != NULL);

    pb->http_buf_len = snprintf(
        pb->http_buf,
        sizeof pb->http_buf,
        "/v1/channel-registration/sub-key/%s/channel-group/%s/remove?pnsdk=%s",
        pb->subscribe_key,
        channel_group,
        pubnub_uname());
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');

    return PNR_STARTED;
}


enum pubnub_res pbcc_channel_registry_prep(struct pbcc_context* pb,
                                           char const*          channel_group,
                                           char const*          param,
                                           char const*          channel)
{
    PUBNUB_ASSERT_OPT(channel_group != NULL);

    pb->http_buf_len = snprintf(
        pb->http_buf,
        sizeof pb->http_buf,
        "/v1/channel-registration/sub-key/%s/channel-group/%s?pnsdk=%s",
        pb->subscribe_key,
        channel_group,
        pubnub_uname());
    if (NULL != param) {
        enum pubnub_res rslt;
        PUBNUB_ASSERT_OPT(channel != NULL);
        rslt = pbcc_append_url_param_encoded(pb, param, strlen(param), channel, '&');
        if (rslt != PNR_OK) {
            return rslt;
        }
    }
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');

    return PNR_STARTED;
}
