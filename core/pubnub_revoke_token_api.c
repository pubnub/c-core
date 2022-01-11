/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#include "core/pubnub_ccore.h"
#include "core/pubnub_netcore.h"
#include "core/pubnub_assert.h"
#include "core/pubnub_timers.h"
#include "core/pubnub_helper.h"
#include "core/pubnub_log.h"
#include "lib/pb_strnlen_s.h"
#include "pbcc_revoke_token_api.h"
#include "pubnub_revoke_token_api.h"
#include "core/pubnub_api_types.h"

#include "core/pbpal.h"


enum pubnub_res pubnub_revoke_token(pubnub_t* pb, char const* token)
{
    enum pubnub_res rslt;
    
    pubnub_mutex_lock(pb->monitor);
    if(!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }

    pb->trans  = PBTT_REVOKE_TOKEN;
    pb->method = pubnubUseDELETE;
    rslt       = pbcc_revoke_token_prep(&pb->core, token, pb->trans);

    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_REVOKE_TOKEN;
        pb->core.last_result = PNR_STARTED;
        pb->method           = pubnubUseDELETE;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }

    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


pubnub_chamebl_t pubnub_get_revoke_token_response(pubnub_t* pb)
{
    pubnub_chamebl_t result;
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (pb->trans != PBTT_REVOKE_TOKEN) {
        PUBNUB_LOG_ERROR("pubnub_get_revoke_token(pb=%p) can be called only if "
                         "previous transaction is PBTT_REVOKE_TOKEN(%d). "
                         "Previous transaction was: %d\n",
                         pb,
                         PBTT_REVOKE_TOKEN,
                         pb->trans);
        pubnub_mutex_unlock(pb->monitor);
        result.ptr = NULL;
        result.size = 0;
        return result;
    }
    result = pbcc_get_revoke_token_response(&pb->core);
    pubnub_mutex_unlock(pb->monitor);

    return result;
}
