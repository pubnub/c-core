/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#if PUBNUB_USE_RETRY_CONFIGURATION
#include "core/pubnub_retry_configuration_internal.h"
#include "core/pubnub_pubsubapi.h"
#endif // #if PUBNUB_USE_RETRY_CONFIGURATION

#include "pbntf_trans_outcome_common.h"

#include "pubnub_assert.h"
#include "pubnub_helper.h"
#ifdef PUBNUB_NTF_RUNTIME_SELECTION
#include "pubnub_ntf_enforcement.h"
#endif


#if PUBNUB_NTF_RUNTIME_SELECTION
#define MAYBE_INLINE
#else
#if __STDC_VERSION__ >= 199901L
#define MAYBE_INLINE static inline
#else
#define MAYBE_INLINE static
#endif
#endif // 1


void pbntf_trans_outcome(pubnub_t* pb, enum pubnub_state state)
{
    PBNTF_TRANS_OUTCOME_COMMON(pb, state);
    PUBNUB_ASSERT(pbnc_can_start_transaction(pb));

    pb->flags.sent_queries = 0;
#if PUBNUB_USE_RETRY_CONFIGURATION
    if (NULL != pb->core.retry_configuration &&
        pubnub_retry_configuration_retryable_result_(pb)) {
        uint16_t delay = pubnub_retry_configuration_delay_(pb);

        if (delay > 0) {
            if (NULL == pb->core.retry_timer)
                pb->core.retry_timer = pbcc_request_retry_timer_alloc(pb);
            if (NULL != pb->core.retry_timer) {
                PUBNUB_LOG_WARNING(
                    pb,
                    "HTTP request retry #%d in %ums.",
                    pb->core.http_retry_count + 1,
                    delay);
                pbcc_request_retry_timer_start(pb->core.retry_timer, delay);
                return;
            }
        }
    }

    /** There were no need to start retry timer, we can free it if exists. */
    if (NULL != pb->core.retry_timer) {
        pb->core.http_retry_count = 0;
        pbcc_request_retry_timer_free(&pb->core.retry_timer);
    }
#endif // #if PUBNUB_USE_RETRY_CONFIGURATION
    if (pb->cb != NULL) {
        PUBNUB_LOG_TRACE(
            pb,
            "Call callback for %d transaction outcome: %s",
            pb->trans,
            pubnub_res_2_string(pb->core.last_result));
        pb->cb(pb, pb->trans, pb->core.last_result, pb->user_data);
    }
}

MAYBE_INLINE void pbnc_tr_cxt_state_reset_callback(pubnub_t* pb)
{
    if (pb->trans == PBTT_SET_STATE) {
        PUBNUB_LOG_DEBUG(
            pb, "Reset presence state after %d transaction.", pb->trans);
        if (pb->core.state) {
            free((char*)pb->core.state);
            pb->core.state = NULL;
        }
    }
}

MAYBE_INLINE enum pubnub_res pubnub_last_result_callback(pubnub_t* pb)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    rslt = pb->core.last_result;
    if (rslt != PNR_OK) { pbnc_tr_cxt_state_reset_callback(pb); }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


#if !defined(PUBNUB_NTF_RUNTIME_SELECTION)
void pbnc_tr_cxt_state_reset(pubnub_t* pb)
{
    pbnc_tr_cxt_state_reset_callback(pb);
}


enum pubnub_res pubnub_last_result(pubnub_t* pb)
{
    return pubnub_last_result_callback(pb);
}
#endif

enum pubnub_res pubnub_register_callback(
    pubnub_t*         pb,
    pubnub_callback_t cb,
    void*             user_data)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    pubnub_mutex_lock(pb->monitor);
    pb->cb        = cb;
    pb->user_data = user_data;
    pubnub_mutex_unlock(pb->monitor);
    return PNR_OK;
}


void* pubnub_get_user_data(pubnub_t* pb)
{
    void* result;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    result = pb->user_data;
    pubnub_mutex_unlock(pb->monitor);

    return result;
}


pubnub_callback_t pubnub_get_callback(pubnub_t* pb)
{
    pubnub_callback_t result;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    result = pb->cb;
    pubnub_mutex_unlock(pb->monitor);

    return result;
}
