/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_ntf_sync.h"

#include "pubnub_internal.h"
#include "pbpal.h"
#include "pubnub_assert.h"
#include "pubnub_log.h"

#include "pbntf_trans_outcome_common.h"

#include "lib/msstopwatch/msstopwatch.h"


int pbntf_init(void)
{
    return 0;
}


int pbntf_got_socket(pubnub_t* pb)
{
#if PUBNUB_BLOCKING_IO_SETTABLE
    pbpal_set_blocking_io(pb);
#endif
    return +1;
}


void pbntf_update_socket(pubnub_t* pb)
{
    PUBNUB_UNUSED(pb);
}


void pbntf_start_transaction_timer(pubnub_t* pb)
{
    PUBNUB_UNUSED(pb);
}


void pbntf_start_wait_connect_timer(pubnub_t* pb)
{
    PUBNUB_UNUSED(pb);
}


void pbntf_lost_socket(pubnub_t* pb)
{
    PUBNUB_UNUSED(pb);
}


void pbntf_trans_outcome(pubnub_t* pb, enum pubnub_state state)
{
    PBNTF_TRANS_OUTCOME_COMMON(pb, state);
    PUBNUB_ASSERT(pbnc_can_start_transaction(pb));
}


int pbntf_enqueue_for_processing(pubnub_t* pb)
{
    PUBNUB_UNUSED(pb);
    return 0;
}


int pbntf_requeue_for_processing(pubnub_t* pb)
{
    PUBNUB_UNUSED(pb);

    return 0;
}


int pbntf_watch_in_events(pubnub_t* pbp)
{
    PUBNUB_UNUSED(pbp);
    return 0;
}


int pbntf_watch_out_events(pubnub_t* pbp)
{
    PUBNUB_UNUSED(pbp);
    return 0;
}

void pbnc_tr_cxt_state_reset(pubnub_t* pb)
{
    if (pb->trans == PBTT_SET_STATE)
    {
        PUBNUB_LOG_DEBUG("ntf_sync pbnc_tr_cxt_state_reset. pb->trans=%d\n", pb->trans);
        if (pb->core.state){
            free((char*)pb->core.state);
            pb->core.state = NULL;
        }
    }
}

enum pubnub_res pubnub_last_result(pubnub_t* pb)
{
    enum pubnub_res result;
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pbnc_fsm((pubnub_t*)pb);
    }
    result = pb->core.last_result;
    if (result != PNR_OK){
        pbnc_tr_cxt_state_reset(pb);
    }
    pubnub_mutex_unlock(pb->monitor);

    return result;
}

enum pubnub_res pubnub_await(pubnub_t* pb)
{
    pbmsref_t       t0;
    enum pubnub_res result;
    bool            stopped = false;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    t0 = pbms_start();
    while (!pbnc_can_start_transaction(pb)) {
        pbms_t delta;

        pbnc_fsm(pb);

        delta = pbms_elapsed(t0);
        if (delta > pb->transaction_timeout_ms) {
            if (!stopped) {
                pbnc_stop(pb, PNR_TIMEOUT);
                t0      = pbms_start();
                stopped = true;
            }
            else {
                break;
            }
        }
    }
    result = pb->core.last_result;
    if (result != PNR_OK){
        pbnc_tr_cxt_state_reset(pb);
    }
    pubnub_mutex_unlock(pb->monitor);

    return result;
}
