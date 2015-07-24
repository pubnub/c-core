/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_ntf_sync.h"

#include "pbpal.h"
#include "pubnub_internal.h"
#include "pubnub_assert.h"

#include "pbntf_trans_outcome_common.h"


int pbntf_init(void)
{
    return 0;
}


int pbntf_got_socket(pubnub_t *pb, pb_socket_t socket)
{
    if (PUBNUB_BLOCKING_IO_SETTABLE) {
        pbpal_set_blocking_io(pb);
    }
    return 0;
}


void pbntf_lost_socket(pubnub_t *pb, pb_socket_t socket)
{
}


void pbntf_trans_outcome(pubnub_t *pb, enum pubnub_res result)
{
    PBNTF_TRANS_OUTCOME_COMMON(pb, result);
}


enum pubnub_res pubnub_last_result(pubnub_t const *pb)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    pbnc_fsm((pubnub_t*)pb);
    return pb->core.last_result;
}


enum pubnub_res pubnub_await(pubnub_t *pb)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    while (PNR_STARTED == pb->core.last_result) {
        pbnc_fsm(pb);
    }
    return pb->core.last_result;
}
