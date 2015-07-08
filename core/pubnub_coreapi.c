/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_coreapi.h"

#include "pubnub_ccore.h"
#include "pubnub_netcore.h"
#include "pubnub_internal.h"
#include "pubnub_assert.h"

#include "pbpal.h"


void pubnub_init(pubnub_t *p, const char *publish_key, const char *subscribe_key)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));

    pbcc_init(&p->core, publish_key, subscribe_key);
    p->state = PBS_IDLE;
    p->trans = PBTT_NONE;
    pbpal_init(p);
}


enum pubnub_res pubnub_publish(pubnub_t *pb, const char *channel, const char *message)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    
    if (pb->state != PBS_IDLE) {
        return PNR_IN_PROGRESS;
    }

    rslt = pbcc_publish_prep(&pb->core, channel, message, false, false);
    if (PNR_STARTED == rslt) {
        pb->trans = PBTT_PUBLISH;
        pb->core.last_result = PNR_STARTED;
	pbnc_fsm(pb);
    }
    
    return rslt;
}


enum pubnub_res pubnub_publishv2(pubnub_t *pb, const char *channel, const char *message, bool store_in_history, bool eat_after_reading)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    
    if (pb->state != PBS_IDLE) {
        return PNR_IN_PROGRESS;
    }

    rslt = pbcc_publish_prep(&pb->core, channel, message, store_in_history, eat_after_reading);
    if (PNR_STARTED == rslt) {
        pb->trans = PBTT_PUBLISH;
        pb->core.last_result = PNR_STARTED;
	pbnc_fsm(pb);
    }
    
    return rslt;
}


char const *pubnub_get(pubnub_t *pb)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    return pbcc_get_msg(&pb->core);
}


char const *pubnub_get_channel(pubnub_t *pb)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    return pbcc_get_channel(&pb->core);
}


enum pubnub_res pubnub_subscribe(pubnub_t *p, const char *channel, const char *channel_group)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));
    
    if (p->state != PBS_IDLE) {
        return PNR_IN_PROGRESS;
    }
    
    rslt = pbcc_subscribe_prep(&p->core, channel, channel_group);
    if (PNR_STARTED == rslt) {
        p->trans = PBTT_SUBSCRIBE;
        p->core.last_result = PNR_STARTED;
	pbnc_fsm(p);
    }
    
    return rslt;
}


enum pubnub_res pubnub_leave(pubnub_t *p, const char *channel, const char *channel_group)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));

    if (p->state != PBS_IDLE) {
        return PNR_IN_PROGRESS;
    }
    
    rslt = pbcc_leave_prep(&p->core, channel, channel_group);
    if (PNR_STARTED == rslt) {
        p->trans = PBTT_LEAVE;
        p->core.last_result = PNR_STARTED;
	pbnc_fsm(p);
    }
    
    return rslt;
}


enum pubnub_res pubnub_time(pubnub_t *p)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));

    if (p->state != PBS_IDLE) {
        return PNR_IN_PROGRESS;
    }
    
    rslt = pbcc_time_prep(&p->core);
    if (PNR_STARTED == rslt) {
        p->trans = PBTT_TIME;
        p->core.last_result = PNR_STARTED;
	pbnc_fsm(p);
    }
    
    return rslt;
}


enum pubnub_res pubnub_history(pubnub_t *pb, const char *channel, const char *channel_group, unsigned count)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    if (pb->state != PBS_IDLE) {
        return PNR_IN_PROGRESS;
    }
    
    rslt = pbcc_history_prep(&pb->core, channel, channel_group, count);
    if (PNR_STARTED == rslt) {
        pb->trans = PBTT_HISTORY;
        pb->core.last_result = PNR_STARTED;
	pbnc_fsm(pb);
    }
    
    return rslt;
}


enum pubnub_res pubnub_historyv2(pubnub_t *pb, const char *channel, const char *channel_group, unsigned count, bool include_token)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    if (pb->state != PBS_IDLE) {
        return PNR_IN_PROGRESS;
    }
    
    rslt = pbcc_historyv2_prep(&pb->core, channel, channel_group, count, include_token);
    if (PNR_STARTED == rslt) {
        pb->trans = PBTT_HISTORYV2;
        pb->core.last_result = PNR_STARTED;
	pbnc_fsm(pb);
    }
    
    return rslt;
}


enum pubnub_res pubnub_here_now(pubnub_t *pb, const char *channel, const char *channel_group)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    if (pb->state != PBS_IDLE) {
        return PNR_IN_PROGRESS;
    }
    
    rslt = pbcc_here_now_prep(&pb->core, channel, channel_group);
    if (PNR_STARTED == rslt) {
        pb->trans = PBTT_HERENOW;
        pb->core.last_result = PNR_STARTED;
	pbnc_fsm(pb);
    }
    
    return rslt;
}


enum pubnub_res pubnub_global_here_now(pubnub_t *pb)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    if (pb->state != PBS_IDLE) {
        return PNR_IN_PROGRESS;
    }
    
    rslt = pbcc_here_now_prep(&pb->core, NULL, NULL);
    if (PNR_STARTED == rslt) {
        pb->trans = PBTT_GLOBAL_HERENOW;
        pb->core.last_result = PNR_STARTED;
	pbnc_fsm(pb);
    }
    
    return rslt;
}


enum pubnub_res pubnub_where_now(pubnub_t *pb, const char *uuid)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    if (pb->state != PBS_IDLE) {
        return PNR_IN_PROGRESS;
    }
    
    rslt = pbcc_where_now_prep(&pb->core, uuid ? uuid : pb->core.uuid);
    if (PNR_STARTED == rslt) {
        pb->trans = PBTT_WHERENOW;
        pb->core.last_result = PNR_STARTED;
	pbnc_fsm(pb);
    }
    
    return rslt;
}


enum pubnub_res pubnub_set_state(pubnub_t *pb, char const *channel, char const *channel_group, const char *uuid, char const *state)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    if (pb->state != PBS_IDLE) {
        return PNR_IN_PROGRESS;
    }
    
    rslt = pbcc_set_state_prep(&pb->core, channel, channel_group, uuid ? uuid : pb->core.uuid, state);
    if (PNR_STARTED == rslt) {
        pb->trans = PBTT_SET_STATE;
        pb->core.last_result = PNR_STARTED;
	pbnc_fsm(pb);
    }
    
    return rslt;
}


enum pubnub_res pubnub_state_get(pubnub_t *pb, char const *channel, char const *channel_group, const char *uuid)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    if (pb->state != PBS_IDLE) {
        return PNR_IN_PROGRESS;
    }
    
    rslt = pbcc_state_get_prep(&pb->core, channel, channel_group, uuid ? uuid : pb->core.uuid);
    if (PNR_STARTED == rslt) {
        pb->trans = PBTT_STATE_GET;
        pb->core.last_result = PNR_STARTED;
	pbnc_fsm(pb);
    }
    
    return rslt;
}


void pubnub_cancel(pubnub_t *pb)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    
    switch (pb->state) {
    case PBS_WAIT_CANCEL:
    case PBS_WAIT_CANCEL_CLOSE:
    case PBS_IDLE:
    case PBS_NULL:
        break;
    case PBS_WAIT_DNS:
        pb->core.msg_ofs = pb->core.msg_end = 0;
        pbntf_trans_outcome(pb, PNR_CANCELLED);
        break;
    default:
        pb->state = PBS_WAIT_CANCEL;
        break;                        
    }
}


void pubnub_set_uuid(pubnub_t *pb, const char *uuid)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    pbcc_set_uuid(&pb->core, uuid);
}


char const *pubnub_get_uuid(pubnub_t *pb)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    return pb->core.uuid;
}


void pubnub_set_auth(pubnub_t *pb, const char *auth)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    pbcc_set_auth(&pb->core, auth);
}


int pubnub_last_http_code(pubnub_t const *pb)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    return pb->core.http_code;
}


char const *pubnub_last_time_token(pubnub_t const *pb)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    return pb->core.timetoken;
}
