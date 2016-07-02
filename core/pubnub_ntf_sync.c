/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_ntf_sync.h"

#include "pbpal.h"
#include "pubnub_internal.h"
#include "pubnub_assert.h"
#include "pubnub_log.h"

#include "pbntf_trans_outcome_common.h"


int pbntf_init(void)
{
    return 0;
}


int pbntf_got_socket(pubnub_t *pb, pb_socket_t socket)
{
    PUBNUB_UNUSED(socket);
    if (PUBNUB_BLOCKING_IO_SETTABLE) {
        pbpal_set_blocking_io(pb);
    }
    return +1;
}


void pbntf_update_socket(pubnub_t *pb, pb_socket_t socket)
{
    PUBNUB_UNUSED(socket);
    PUBNUB_UNUSED(pb);
}


void pbntf_lost_socket(pubnub_t *pb, pb_socket_t socket)
{
    PUBNUB_UNUSED(socket);
    PUBNUB_UNUSED(pb);
}


void pbntf_trans_outcome(pubnub_t *pb)
{
    PBNTF_TRANS_OUTCOME_COMMON(pb);
}


int pbntf_enqueue_for_processing(pubnub_t *pb)
{
    PUBNUB_UNUSED(pb);
    return 0;
}


int pbntf_requeue_for_processing(pubnub_t *pb)
{
    PUBNUB_UNUSED(pb);
    
    return 0;
}


int pbntf_watch_in_events(pubnub_t *pbp)
{
    PUBNUB_UNUSED(pbp);
    return 0;
}


int pbntf_watch_out_events(pubnub_t *pbp)
{
    PUBNUB_UNUSED(pbp);
    return 0;
}


enum pubnub_res pubnub_last_result(pubnub_t *pb)
{
    enum pubnub_res result;
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (pb->state != PBS_IDLE) {
        pbnc_fsm((pubnub_t*)pb);
    }
    result = pb->core.last_result;
    pubnub_mutex_unlock(pb->monitor);

    return result;
}


enum pubnub_res pubnub_await(pubnub_t *pb)
{
    enum pubnub_res result;
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    while (pb->state != PBS_IDLE) {
        pbnc_fsm(pb);
    }
    result = pb->core.last_result;
    pubnub_mutex_unlock(pb->monitor);

    return result;
}