/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_coreapi_ex.h"

#include "pubnub_ccore.h"
#include "pubnub_netcore.h"
#include "pubnub_internal.h"
#include "pubnub_assert.h"
#include "pubnub_timers.h"

#include "pbpal.h"

#include <stdlib.h>
#include <ctype.h>


/** Minimal presence heartbeat interval supported by
    Pubnub, in seconds.
*/
#define PUBNUB_MINIMAL_HEARTBEAT_INTERVAL 270


struct pubnub_subscribe_options pubnub_subscribe_defopts(void)
{
    struct pubnub_subscribe_options result;
    result.channel_group = NULL;
    result.heartbeat = PUBNUB_MINIMAL_HEARTBEAT_INTERVAL;
    return result;
}


enum pubnub_res pubnub_subscribe_ex(pubnub_t *p, const char *channel, struct pubnub_subscribe_options opt)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));
    
    pubnub_mutex_lock(p->monitor);
    if (p->state != PBS_IDLE) {
        pubnub_mutex_unlock(p->monitor);
        return PNR_IN_PROGRESS;
    }
    
    rslt = pbcc_subscribe_prep(&p->core, channel, opt.channel_group, &opt.heartbeat);
    if (PNR_STARTED == rslt) {
        p->trans = PBTT_SUBSCRIBE;
        p->core.last_result = PNR_STARTED;
        pbnc_fsm(p);
        rslt = p->core.last_result;
    }
    
    pubnub_mutex_unlock(p->monitor);
    return rslt;
}


struct pubnub_here_now_options pubnub_here_now_defopts(void)
{
    struct pubnub_here_now_options result;
    result.channel_group = NULL;
    result.disable_uuids = false;
    result.state = false;
    return result;
}


enum pubnub_res pubnub_here_now_ex(pubnub_t *pb, const char *channel, struct pubnub_here_now_options opt)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (pb->state != PBS_IDLE) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    
    rslt = pbcc_here_now_prep(
        &pb->core, 
        channel, 
        opt.channel_group,
        opt.disable_uuids ? pbccTrue : pbccFalse,
        opt.state ? pbccTrue : pbccFalse
        );
    if (PNR_STARTED == rslt) {
        pb->trans = PBTT_HERENOW;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    
    pubnub_mutex_unlock(pb->monitor);
    return rslt;
}


enum pubnub_res pubnub_global_here_now_ex(pubnub_t *pb, struct pubnub_here_now_options opt)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT(NULL == opt.channel_group);

    pubnub_mutex_lock(pb->monitor);
    if (pb->state != PBS_IDLE) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    
    rslt = pbcc_here_now_prep(
        &pb->core, 
        NULL, 
        NULL,
        opt.disable_uuids ? pbccTrue : pbccFalse,
        opt.state ? pbccTrue : pbccFalse
        );
    if (PNR_STARTED == rslt) {
        pb->trans = PBTT_HERENOW;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    
    pubnub_mutex_unlock(pb->monitor);
    return rslt;
}

