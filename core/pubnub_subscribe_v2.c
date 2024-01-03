/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if PUBNUB_USE_SUBSCRIBE_V2

#include "pubnub_internal.h"

#include "pubnub_server_limits.h"
#include "pubnub_subscribe_v2.h"
#include "pbcc_subscribe_v2.h"
#include "pubnub_ccore_pubsub.h"
#include "pubnub_netcore.h"
#include "pubnub_assert.h"
#include "pubnub_log.h"
#include "pubnub_version.h"
#include "pubnub_json_parse.h"

#include "pbpal.h"


struct pubnub_subscribe_v2_options pubnub_subscribe_v2_defopts(void)
{
    struct pubnub_subscribe_v2_options result;
    result.channel_group = NULL;
    result.heartbeat     = PUBNUB_MINIMAL_HEARTBEAT_INTERVAL;
    result.filter_expr   = NULL;
    return result;
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
    rslt = pbauto_heartbeat_prepare_channels_and_ch_groups(p, &channel, &opt.channel_group);
    if (rslt != PNR_OK) {
        return rslt;
    }

    rslt = pbcc_subscribe_v2_prep(
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


struct pubnub_v2_message pubnub_get_v2(pubnub_t* pb)
{
    struct pubnub_v2_message result;
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    result = pbcc_get_msg_v2(&pb->core);
    pubnub_mutex_unlock(pb->monitor);

    return result;
}

#endif /* PUBNUB_USE_SUBSCRIBE_V2 */

