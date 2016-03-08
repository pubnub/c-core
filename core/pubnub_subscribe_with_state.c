/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_coreapi.h"

#include "pubnub_netcore.h"
#include "pubnub_internal.h"
#include "pubnub_assert.h"
#include "pubnub_version.h"

#include <string.h>
#include <stdio.h>


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

static enum pubnub_res pbcc_subscribe_with_state_prep(struct pbcc_context *p, const char *channel, const char *channel_group, char const *state)
{
    char const *pmessage = state;

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
        "/subscribe/%s/%s/0/%s?pnsdk=%s&state=",
        p->subscribe_key, channel, p->timetoken,
        pubnub_uname()
        );
    while (pmessage[0]) {
        /* RFC 3986 Unreserved characters plus few
         * safe reserved ones. */
        size_t okspan = strspn(pmessage, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.~" ",=:;@[]");
        if (okspan > 0) {
            if (okspan > sizeof(p->http_buf)-1 - p->http_buf_len) {
                p->http_buf_len = 0;
                return PNR_TX_BUFF_TOO_SMALL;
            }
            memcpy(p->http_buf + p->http_buf_len, pmessage, okspan);
            p->http_buf_len += okspan;
            p->http_buf[p->http_buf_len] = 0;
            pmessage += okspan;
        }
        if (pmessage[0]) {
            /* %-encode a non-ok character. */
            char enc[4] = {'%'};
            enc[1] = "0123456789ABCDEF"[pmessage[0] / 16];
            enc[2] = "0123456789ABCDEF"[pmessage[0] % 16];
            if (3 > sizeof p->http_buf - 1 - p->http_buf_len) {
                p->http_buf_len = 0;
                return PNR_TX_BUFF_TOO_SMALL;
            }
            memcpy(p->http_buf + p->http_buf_len, enc, 4);
            p->http_buf_len += 3;
            ++pmessage;
        }
    }
    APPEND_URL_PARAM_M(p, "channel-group", channel_group, '&');
    APPEND_URL_PARAM_M(p, "uuid", p->uuid, '&');
    APPEND_URL_PARAM_M(p, "auth", p->auth, '&');

    printf("===========\np->http_buf:%s\n=========", p->http_buf);

    return PNR_STARTED;
}


enum pubnub_res pubnub_subscribe_with_state(pubnub_t *p, const char *channel, const char *channel_group, char const *state)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));
    
    pubnub_mutex_lock(p->monitor);
    if (p->state != PBS_IDLE) {
        pubnub_mutex_unlock(p->monitor);
        return PNR_IN_PROGRESS;
    }
    
    rslt = pbcc_subscribe_with_state_prep(&p->core, channel, channel_group, state);
    if (PNR_STARTED == rslt) {
        p->trans = PBTT_SUBSCRIBE;
        p->core.last_result = PNR_STARTED;
        pbnc_fsm(p);
        rslt = p->core.last_result;
    }
    
    pubnub_mutex_unlock(p->monitor);
    return rslt;
}
