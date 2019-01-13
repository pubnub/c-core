/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#include "pubnub_subscribe_v2.h"

#include "pubnub_ccore_pubsub.h"
#include "pubnub_netcore.h"
#include "pubnub_assert.h"
#include "pubnub_log.h"
#include "pubnub_version.h"
#include "pubnub_json_parse.h"

#include "pbpal.h"


/** It has to contain the `t` field, with another `t` for the
    timetoken (as string) and `tr` for the region (integer) and the
    `m` field for the message(s) array.
*/
#define MIN_SUBSCRIBE_V2_RESPONSE_LENGTH 40

/** Minimal presence heartbeat interval supported by
    Pubnub, in seconds.
*/
#define PUBNUB_MINIMAL_HEARTBEAT_INTERVAL 270


struct pubnub_subscribe_v2_options pubnub_subscribe_v2_defopts(void)
{
    struct pubnub_subscribe_v2_options result;
    result.channel_group = NULL;
    result.heartbeat     = PUBNUB_MINIMAL_HEARTBEAT_INTERVAL;
    result.filter_expr   = NULL;
    return result;
}


static enum pubnub_res subscribe_v2_prep(struct pbcc_context* p,
                                         char const*          channel,
                                         char const*          channel_group,
                                         unsigned*            heartbeat,
                                         char const*          filter_expr)
{
    char        region_str[20];
    char const* tr;

    if (NULL == channel) {
        if (NULL == channel_group) {
            return PNR_INVALID_CHANNEL;
        }
        channel = ",";
    }
    if (p->msg_ofs < p->msg_end) {
        return PNR_RX_BUFF_NOT_EMPTY;
    }

    if ('\0' == p->timetoken[0]) {
        p->timetoken[0] = '0';
        p->timetoken[1] = '\0';
        tr              = NULL;
    }
    else {
        snprintf(region_str, sizeof region_str, "%d", p->region);
        tr = region_str;
    }
    p->http_content_len = 0;
    p->msg_ofs = p->msg_end = 0;

    p->http_buf_len = snprintf(p->http_buf,
                               sizeof p->http_buf,
                               "/v2/subscribe/%s/",
                               p->subscribe_key);
    APPEND_URL_ENCODED_M(p, channel);
    p->http_buf_len += snprintf(p->http_buf + p->http_buf_len,
                                sizeof p->http_buf - p->http_buf_len,
                                "/0?tt=%s&pnsdk=%s",
                                p->timetoken,
                                pubnub_uname());
    APPEND_URL_PARAM_M(p, "tr", tr, '&');
    APPEND_URL_PARAM_M(p, "channel-group", channel_group, '&');
    APPEND_URL_PARAM_M(p, "uuid", p->uuid, '&');
    APPEND_URL_PARAM_M(p, "auth", p->auth, '&');
    APPEND_URL_PARAM_ENCODED_M(p, "filter-expr", filter_expr, '&');
    APPEND_URL_OPT_PARAM_UNSIGNED_M(p, "heartbeat", heartbeat, '&');

    return PNR_STARTED;
}


enum pubnub_res pubnub_subscribe_v2(pubnub_t*                          p,
                                    const char*                        channel,
                                    struct pubnub_subscribe_v2_options opt)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));

    pubnub_mutex_lock(p->monitor);
    if (!pbnc_can_start_transaction(p)) {
        pubnub_mutex_unlock(p->monitor);
        return PNR_IN_PROGRESS;
    }

    rslt = subscribe_v2_prep(
        &p->core, channel, opt.channel_group, &opt.heartbeat, opt.filter_expr);
    if (PNR_STARTED == rslt) {
        p->trans            = PBTT_SUBSCRIBE_V2;
        p->core.last_result = PNR_STARTED;
        pbnc_fsm(p);
        rslt = p->core.last_result;
    }

    pubnub_mutex_unlock(p->monitor);
    return rslt;
}


enum pubnub_res pbcc_parse_subscribe_v2_response(struct pbcc_context* p)
{
    enum pbjson_object_name_parse_result jpresult;
    struct pbjson_elem                   el;
    struct pbjson_elem                   found;
    char*                                reply    = p->http_reply;
    int                                  replylen = p->http_buf_len;

    if (replylen < MIN_SUBSCRIBE_V2_RESPONSE_LENGTH) {
        return PNR_FORMAT_ERROR;
    }
    if ((reply[0] != '{') || (reply[replylen - 1] != '}')) {
        return PNR_FORMAT_ERROR;
    }

    el.start = p->http_reply;
    el.end   = p->http_reply + p->http_buf_len;
    jpresult = pbjson_get_object_value(&el, "t", &found);
    if (jonmpOK == jpresult) {
        struct pbjson_elem titel;
        if (jonmpOK == pbjson_get_object_value(&found, "t", &titel)) {
            unsigned len = titel.end - titel.start - 2;
            if ((*titel.start != '"') || (titel.end[-1] != '"')) {
                PUBNUB_LOG_ERROR("Time token in response is not a string\n");
                return PNR_FORMAT_ERROR;
            }
            if (len >= sizeof p->timetoken) {
                PUBNUB_LOG_ERROR("Time token in response has length %u, longer "
                                 "than our max %lu\n",
                                 len,
                                 sizeof p->timetoken - 1);
                return PNR_FORMAT_ERROR;
            }

            memcpy(p->timetoken, titel.start + 1, len);
            p->timetoken[len] = '\0';
        }
        else {
            PUBNUB_LOG_ERROR(
                "No timetoken value in subscribe V2 response found\n");
            return PNR_FORMAT_ERROR;
        }
        if (jonmpOK == pbjson_get_object_value(&found, "r", &titel)) {
            char s[20];
            pbjson_element_strcpy(&titel, s, sizeof s);
            p->region = strtol(s, NULL, 0);
        }
        else {
            PUBNUB_LOG_ERROR(
                "No region value in subscribe V2 response found\n");
            return PNR_FORMAT_ERROR;
        }
    }
    else {
        PUBNUB_LOG_ERROR(
            "No timetoken in subscribe V2 response found, error=%d\n", jpresult);
        return PNR_FORMAT_ERROR;
    }

    p->chan_ofs = p->chan_end = 0;

    /* We should optimize this to not look from the start again.
     */
    jpresult = pbjson_get_object_value(&el, "m", &found);
    if (jonmpOK == jpresult) {
        p->msg_ofs = found.start - reply + 1;
        p->msg_end = found.end - reply - 1;
    }
    else {
        PUBNUB_LOG_ERROR(
            "No message array subscribe V2 response found, error=%d\n", jpresult);
        return PNR_FORMAT_ERROR;
    }

    return PNR_OK;
}


struct pubnub_v2_message pubnub_get_v2(pubnub_t* pbp)
{
    enum pbjson_object_name_parse_result jpresult;
    struct pbjson_elem                   el;
    struct pbjson_elem                   found;
    struct pubnub_v2_message             rslt;
    struct pbcc_context*                 p = &pbp->core;
    char const*                          start;
    char const*                          end;
    char const*                          seeker;

    memset(&rslt, 0, sizeof rslt);

    if (p->msg_ofs >= p->msg_end) {
        return rslt;
    }
    start = p->http_reply + p->msg_ofs;
    if (*start != '{') {
        PUBNUB_LOG_ERROR(
            "Message subscribe V2 response is not a JSON object\n");
        return rslt;
    }
    end    = p->http_reply + p->msg_end;
    seeker = pbjson_find_end_complex(start, end);
    if (seeker == end) {
        PUBNUB_LOG_ERROR(
            "Message subscribe V2 response has no endo of JSON object\n");
        return rslt;
    }

    p->msg_ofs = seeker - p->http_reply + 2;
    el.start   = start;
    el.end     = seeker;

    /* @todo We should iterate over elements of JSON message object,
       instead of looking from the start, again and again.
    */

    jpresult = pbjson_get_object_value(&el, "d", &found);
    if (jonmpOK == jpresult) {
        rslt.payload.ptr  = (char*)found.start;
        rslt.payload.size = found.end - found.start;
    }
    else {
        PUBNUB_LOG_ERROR("pbp=%p: No message payload in subscribe V2 response "
                         "found, error=%d\n",
                         pbp,
                         jpresult);
        return rslt;
    }

    jpresult = pbjson_get_object_value(&el, "c", &found);
    if (jonmpOK == jpresult) {
        rslt.channel.ptr  = (char*)found.start + 1;
        rslt.channel.size = found.end - found.start - 2;
    }
    else {
        PUBNUB_LOG_ERROR("pbp=%p: No message channel in subscribe V2 response "
                         "found, error=%d\n",
                         pbp,
                         jpresult);
        return rslt;
    }

    jpresult = pbjson_get_object_value(&el, "p", &found);
    if (jonmpOK == jpresult) {
        struct pbjson_elem titel;
        if (jonmpOK == pbjson_get_object_value(&found, "t", &titel)) {
            if ((*titel.start != '"') || (titel.end[-1] != '"')) {
                PUBNUB_LOG_ERROR("Time token in response is not a string\n");
                return rslt;
            }
            rslt.tt.ptr  = (char*)titel.start + 1;
            rslt.tt.size = titel.end - titel.start - 2;
        }
        else {
            PUBNUB_LOG_ERROR(
                "No timetoken value in subscribe V2 response found\n");
            return rslt;
        }
    }
    else {
        PUBNUB_LOG_ERROR("No message publish timetoken in subscribe V2 "
                         "response found, error=%d\n",
                         jpresult);
        return rslt;
    }

    if (jonmpOK == pbjson_get_object_value(&el, "b", &found)) {
        rslt.match_or_group.ptr  = (char*)found.start;
        rslt.match_or_group.size = found.end - found.start;
    }

    if (jonmpOK == pbjson_get_object_value(&el, "u", &found)) {
        rslt.metadata.ptr  = (char*)found.start;
        rslt.metadata.size = found.end - found.start;
    }

    return rslt;
}
