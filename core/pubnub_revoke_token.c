/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_revoke_token.h"
#include "pubnub_internal.h"
#include "pbcc_revoke_token.h"


enum pubnub_res pubnub_revoke_token(pubnub_t* pb, char const* token)
{
    enum pubnub_res rslt;
    
    pubnub_mutex_lock(pb->monitor);
    if(!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }

    pb->method = pubnubUseDELETE;
    pb->trans = PBTT_REVOKE_TOKEN;

    rslt = pbcc_revoke_token_prep(&pb->core, token)

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
