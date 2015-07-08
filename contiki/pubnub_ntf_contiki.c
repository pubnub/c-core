/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_ntf_contiki.h"

#include "pubnub_internal.h"
#include "pubnub_assert.h"

#include "pbntf_trans_outcome_common.h"

#include "contiki-net.h"


process_event_t pubnub_publish_event;
process_event_t pubnub_subscribe_event;
process_event_t pubnub_time_event;
process_event_t pubnub_history_event;
process_event_t pubnub_presence_event;


static process_event_t trans2event(enum pubnub_trans trans)
{
    switch (trans) {
    case PBTT_SUBSCRIBE:
        return pubnub_subscribe_event;
    case PBTT_PUBLISH:
        return pubnub_publish_event;
    case PBTT_TIME:
        return pubnub_time_event;
    case PBTT_HISTORY:
    case PBTT_HISTORYV2:
        return pubnub_history_event;
    case PBTT_LEAVE:
    case PBTT_HERENOW:
    case PBTT_GLOBAL_HERENOW:
    case PBTT_WHERENOW:
    case PBTT_SET_STATE:
    case PBTT_STATE_GET:
        return pubnub_presence_event;
    case PBTT_NONE:
    default:
        PUBNUB_ASSERT_OPT(0);
        return PROCESS_EVENT_NONE;
    }
}


void pbntf_trans_outcome(pubnub_t *pb, enum pubnub_res result)
{
    PBNTF_TRANS_OUTCOME_COMMON(pb, result);
    process_post(pb->initiator, trans2event(pb->trans), pb);
}


enum pubnub_res pubnub_last_result(pubnub_t const *pb)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    return pb->core.last_result;
}
