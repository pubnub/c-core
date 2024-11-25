/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#if PUBNUB_USE_FETCH_HISTORY
#include "pubnub_memory_block.h"
#include "pubnub_fetch_history.h"
#include "pubnub_json_parse.h"

#include "pbcc_fetch_history.h"
#include "pubnub_assert.h"
#include "pubnub_log.h"


struct pubnub_fetch_history_options pubnub_fetch_history_defopts(void)
{
    struct pubnub_fetch_history_options rslt;

    rslt.max_per_channel = 0;
    rslt.reverse       = false;
    rslt.start         = NULL;
    rslt.end           = NULL;
    rslt.include_meta  = false;
    rslt.include_message_type = false;
    rslt.include_user_id = false;
    rslt.include_message_actions = false;
    rslt.include_custom_message_type = false;

    return rslt;
}

enum pubnub_res pubnub_fetch_history(pubnub_t*                     pb,
                                  char const*                   channel,
                                  struct pubnub_fetch_history_options opt)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }

    rslt = pbcc_fetch_history_prep(&pb->core,
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
        PUBNUB_LOG_ERROR("pubnub_get_fetch_history(pb=%p) can be called only if "
                         "previous transaction is PBTT_FETCH_HISTORY(%d). "
                         "Previous transaction was: %d\n",
                         pb,
                         PBTT_FETCH_HISTORY,
                         pb->trans);
        pubnub_mutex_unlock(pb->monitor);
        result.ptr = NULL;
        result.size = 0;
        return result;
    }
    result = pbcc_get_fetch_history(&pb->core);
    pubnub_mutex_unlock(pb->monitor);

    return result;
}

#endif /* PUBNUB_USE_FETCH_HISTORY */
