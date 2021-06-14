/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#include "core/pubnub_ccore.h"
#include "core/pubnub_netcore.h"
#include "core/pubnub_assert.h"
#include "core/pubnub_timers.h"
#include "core/pubnub_log.h"
#include "lib/pb_strnlen_s.h"
#include "core/pbcc_grant_token_api.h"
#include "core/pubnub_grant_token_api.h"
#include "core/pubnub_api_types.h"

#include "core/pbpal.h"

#include <ctype.h>
#include <string.h>

enum grant_bit_flag { 
    PERM_READ = 1
    , PERM_WRITE = 2
    , PERM_MANAGE = 4
    , PERM_DELETE = 8
    , PERM_CREATE = 16 };


int pubnub_get_grant_bit_mask_value(bool read, 
                                  bool write, 
                                  bool manage, 
                                  bool del, 
                                  bool create)
{
    int result = 0;
    if (read)   { result = PERM_READ; }
    if (write)  { result = result + PERM_WRITE; }
    if (manage) { result = result + PERM_MANAGE; }
    if (del) { result = result + PERM_DELETE; }
    if (create) { result = result + PERM_CREATE; }
    return result;
}

enum pubnub_res pubnub_grant_token(pubnub_t* pb, char const* perm_obj)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    
    pb->method = pubnubSendViaPOST;
    pb->trans = PBTT_GRANT_TOKEN;
    rslt = pbcc_grant_token_prep(&pb->core, perm_obj, pb->trans);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_GRANT_TOKEN;
        pb->core.last_result = PNR_STARTED;
        pb->method           = pubnubSendViaPOST;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}

pubnub_chamebl_t pubnub_get_grant_token(pubnub_t* pb)
{
    pubnub_chamebl_t result;
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (pb->trans != PBTT_GRANT_TOKEN) {
        PUBNUB_LOG_ERROR("pubnub_get_grant_token(pb=%p) can be called only if "
                         "previous transaction is PBTT_GRANT_TOKEN(%d). "
                         "Previous transaction was: %d\n",
                         pb,
                         PBTT_GRANT_TOKEN,
                         pb->trans);
        pubnub_mutex_unlock(pb->monitor);
        result.ptr = NULL;
        result.size = 0;
        return result;
    }
    result = pbcc_get_grant_token(&pb->core);
    pubnub_mutex_unlock(pb->monitor);

    return result;
}