/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#if PUBNUB_USE_FETCH_HISTORY
#include "pubnub_memory_block.h"
#include "pubnub_fetch_history.h"
#include "pubnub_json_parse.h"

#include "pbcc_fetch_history.h"
#include "pubnub_assert.h"


struct pubnub_fetch_history_options pubnub_fetch_history_defopts(void)
{
    struct pubnub_fetch_history_options rslt;

    rslt.max_per_channel             = 0;
    rslt.reverse                     = false;
    rslt.start                       = NULL;
    rslt.end                         = NULL;
    rslt.include_meta                = false;
    rslt.include_message_type        = false;
    rslt.include_user_id             = false;
    rslt.include_message_actions     = false;
    rslt.include_custom_message_type = false;

    return rslt;
}

enum pubnub_res pubnub_fetch_history(
    pubnub_t*                           pb,
    char const*                         channel,
    struct pubnub_fetch_history_options opt)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

#if PUBNUB_LOG_ENABLED(DEBUG)
    if (pubnub_logger_should_log(pb, PUBNUB_LOG_LEVEL_DEBUG)) {
        pubnub_log_value_t data = pubnub_log_value_map_init();
        PUBNUB_LOG_MAP_SET_STRING(&data, channel, channels)
        PUBNUB_LOG_MAP_SET_NUMBER(&data, opt.max_per_channel, max_per_channel)
        PUBNUB_LOG_MAP_SET_BOOL(&data, opt.reverse, reverse)
        PUBNUB_LOG_MAP_SET_STRING(&data, opt.start, start)
        PUBNUB_LOG_MAP_SET_STRING(&data, opt.end, end)
        PUBNUB_LOG_MAP_SET_BOOL(&data, opt.include_meta, include_meta)
        PUBNUB_LOG_MAP_SET_BOOL(
            &data, opt.include_message_type, include_message_type)
        PUBNUB_LOG_MAP_SET_BOOL(&data, opt.include_user_id, include_user_id)
        PUBNUB_LOG_MAP_SET_BOOL(
            &data, opt.include_message_actions, include_message_actions)
        PUBNUB_LOG_MAP_SET_BOOL(
            &data, opt.include_custom_message_type, include_custom_message_type)
        pubnub_log_object(
            pb,
            PUBNUB_LOG_LEVEL_DEBUG,
            PUBNUB_LOG_LOCATION,
            &data,
            "Fetch history with parameters:");
    }
#endif

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        PUBNUB_LOG_DEBUG(
            pb,
            "Unable to start transaction. PubNub context is in %s state.",
            pbcc_state_2_string(pb->state));
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }

    rslt = pbcc_fetch_history_prep(
        &pb->core,
        channel,
        opt.max_per_channel,
        opt.include_meta ? pbccTrue : pbccFalse,
        opt.include_custom_message_type ? pbccTrue : pbccFalse,
        opt.include_message_type ? pbccTrue : pbccFalse,
        opt.include_user_id ? pbccTrue : pbccFalse,
        opt.include_message_actions ? pbccTrue : pbccFalse,
        opt.reverse ? pbccTrue : pbccFalse,
        opt.start,
        opt.end);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_FETCH_HISTORY;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }

    pubnub_mutex_unlock(pb->monitor);
    return rslt;
}


pubnub_chamebl_t pubnub_get_fetch_history(pubnub_t* pb)
{
    pubnub_chamebl_t result;
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (pb->trans != PBTT_FETCH_HISTORY) {
        PUBNUB_LOG_ERROR(
            pb,
            "Unexpected previous transaction (%d) when PBTT_FETCH_HISTORY is "
            "expected.",
            pb->trans);
        pubnub_mutex_unlock(pb->monitor);
        result.ptr  = NULL;
        result.size = 0;
        return result;
    }
    result = pbcc_get_fetch_history(&pb->core);
    pubnub_mutex_unlock(pb->monitor);

    return result;
}

#endif /* PUBNUB_USE_FETCH_HISTORY */
