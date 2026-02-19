/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if PUBNUB_USE_SUBSCRIBE_V2

#include "pubnub_internal.h"

#include "pubnub_server_limits.h"
#include "pubnub_subscribe_v2.h"
#include "pbcc_subscribe_v2.h"
#include "pubnub_ccore_pubsub.h"
#include "pubnub_netcore.h"
#include "pubnub_assert.h"


struct pubnub_subscribe_v2_options pubnub_subscribe_v2_defopts(void)
{
    struct pubnub_subscribe_v2_options result;
    result.channel_group = NULL;
    result.heartbeat     = PUBNUB_MINIMAL_HEARTBEAT_INTERVAL;
    result.filter_expr   = NULL;
    result.timetoken[0]  = '\0';
    return result;
}


enum pubnub_res pubnub_subscribe_v2(
    pubnub_t*                          p,
    const char*                        channel,
    struct pubnub_subscribe_v2_options opts)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));

#if PUBNUB_LOG_ENABLED(DEBUG)
    if (pubnub_logger_should_log(p, PUBNUB_LOG_LEVEL_DEBUG)) {
        pubnub_log_value_t data = pubnub_log_value_map_init();
        PUBNUB_LOG_MAP_SET_STRING(&data, channel, channels)
        PUBNUB_LOG_MAP_SET_STRING(&data, opts.channel_group, channel_groups)
        PUBNUB_LOG_MAP_SET_STRING(&data, opts.timetoken, timetoken)
        PUBNUB_LOG_MAP_SET_NUMBER(&data, opts.heartbeat, heartbeat)
        PUBNUB_LOG_MAP_SET_STRING(&data, opts.filter_expr, filter_expression)
        pubnub_log_object(
            p,
            PUBNUB_LOG_LEVEL_DEBUG,
            PUBNUB_LOG_LOCATION,
            &data,
            "Subscribe with parameters:");
    }
#endif

    pubnub_mutex_lock(p->monitor);
    if (!pbnc_can_start_transaction(p)) {
        PUBNUB_LOG_WARNING(
            p,
            "Unable to start transaction. PubNub context is in %s state",
            pbcc_state_2_string(p->state));
        pubnub_mutex_unlock(p->monitor);
        return PNR_IN_PROGRESS;
    }
    rslt = pbauto_heartbeat_prepare_channels_and_ch_groups(
        p, &channel, &opts.channel_group);
    if (rslt != PNR_OK) { return rslt; }

    rslt = pbcc_subscribe_v2_prep(
        &p->core,
        channel,
        opts.channel_group,
        &opts.heartbeat,
        opts.filter_expr,
        opts.timetoken);
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
