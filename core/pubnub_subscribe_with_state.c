/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#include "pubnub_coreapi.h"
#include "pubnub_netcore.h"
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
                                "/0/%s",
                                p->timetoken);
    URL_PARAMS_INIT(qparam, PUBNUB_MAX_URL_PARAMS);
    char const* const uname = pubnub_uname();
    if (uname) { ADD_URL_PARAM(qparam, pnsdk, uname); }
    if (channel_group) { ADD_URL_PARAM(qparam, channel-group, channel_group); }
    if (p->user_id) { ADD_URL_PARAM(qparam, uuid, p->user_id); }
    if (state) { APPEND_URL_PARAM(qparam, state, state); }

#if PUBNUB_CRYPTO_API
    if (p->secret_key == NULL) { ADD_URL_AUTH_PARAM(p, qparam, auth); }
    ADD_TS_TO_URL_PARAM();
#else
    ADD_URL_AUTH_PARAM(p, qparam, auth);
#endif

#if PUBNUB_CRYPTO_API
  SORT_URL_PARAMETERS(qparam);
#endif
    ENCODE_URL_PARAMETERS(p, qparam);
#if PUBNUB_CRYPTO_API
    if (p->secret_key != NULL) {
        rslt = pbcc_sign_url(p, "", pubnubSendViaGET, true);
    }
#endif
    printf("===========\np->http_buf:%s\n=========", p->http_buf);

    return PNR_STARTED;
}


enum pubnub_res pubnub_subscribe_with_state(pubnub_t *p,
                                            const char *channel,
                                            const char *channel_group,
                                            char const *state)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));
    
    pubnub_mutex_lock(p->monitor);
    if (!pbnc_can_start_transaction(p)) {
        pubnub_mutex_unlock(p->monitor);
        return PNR_IN_PROGRESS;
    }
    rslt = pbauto_heartbeat_prepare_channels_and_ch_groups(p, &channel, &channel_group);
    if (rslt != PNR_OK) {
        return rslt;
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
