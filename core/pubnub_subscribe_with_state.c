/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_coreapi.h"

#include "pubnub_netcore.h"
#include "pubnub_internal.h"
#include "pubnub_assert.h"
#include "pubnub_version.h"
#include "pubnub_url_encode.h"
#include "pubnub_ccore_pubsub.h"

#include <stdio.h>


static enum pubnub_res pbcc_subscribe_with_state_prep(struct pbcc_context *p,
                                                      const char *channel,
                                                      const char *channel_group,
                                                      char const *state)
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
                               sizeof p->http_buf,
                               "/subscribe/%s/",
                               p->subscribe_key);
    APPEND_URL_ENCODED_M(pb, channel);
    p->http_buf_len += snprintf(p->http_buf + p->http_buf_len,
                                sizeof p->http_buf - p->http_buf_len,
                                "/0/%s?pnsdk=%s",
                                p->timetoken,
                                pubnub_uname());
    APPEND_URL_PARAM_ENCODED_M(pb, "state", state, '&');
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
    if (!pbnc_can_start_transaction(pb)) {
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
