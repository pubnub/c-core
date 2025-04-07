/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_ntf_sync.h"
#include "pubnub_api_types.h"
#ifdef PUBNUB_NTF_RUNTIME_SELECTION
#include "pubnub_ntf_enforcement.h"
#endif

#if PUBNUB_USE_RETRY_CONFIGURATION
#include "core/pubnub_retry_configuration_internal.h"
#include "core/pubnub_pubsubapi.h"
#endif // #if PUBNUB_USE_RETRY_CONFIGURATION

#include "pubnub_internal.h"
#include "pbpal.h"
#include "pubnub_assert.h"
#include "pubnub_log.h"

#include "pbntf_trans_outcome_common.h"

#include "lib/msstopwatch/msstopwatch.h"

#include <emscripten.h>


// TODO: decide if it is worth to keep that here 
// 1 - till the flag is fixed
#if 1
#define MAYBE_INLINE 
#else 
#if __STDC_VERSION__ >= 199901L 
#define MAYBE_INLINE static inline 
#else
#define MAYBE_INLINE static 
#endif
#endif // 1


MAYBE_INLINE int pbntf_init_sync(void) 
{
    return 0;
}


MAYBE_INLINE int pbntf_got_socket_sync(pubnub_t* pb)
{
#if PUBNUB_BLOCKING_IO_SETTABLE
    pbpal_set_blocking_io(pb);
#endif
    return +1;
}


MAYBE_INLINE void pbntf_update_socket_sync(pubnub_t* pb)
{
    PUBNUB_UNUSED(pb);
}


MAYBE_INLINE void pbntf_start_transaction_timer_sync(pubnub_t* pb)
{
    PUBNUB_UNUSED(pb);
}


MAYBE_INLINE void pbntf_start_wait_connect_timer_sync(pubnub_t* pb)
{
    PUBNUB_UNUSED(pb);
}


MAYBE_INLINE void pbntf_lost_socket_sync(pubnub_t* pb)
{
    PUBNUB_UNUSED(pb);
}


MAYBE_INLINE void pbntf_trans_outcome_sync(pubnub_t* pb, enum pubnub_state state)
{
    PBNTF_TRANS_OUTCOME_COMMON(pb, state);
    PUBNUB_ASSERT(pbnc_can_start_transaction(pb));
#if PUBNUB_USE_RETRY_CONFIGURATION
    if (NULL != pb->core.retry_configuration &&
        pubnub_retry_configuration_retryable_result_(pb)) {
        uint16_t delay = pubnub_retry_configuration_delay_(pb);

        if (delay > 0) {
            if (NULL == pb->core.retry_timer)
                pb->core.retry_timer = pbcc_request_retry_timer_alloc(pb);
            if (NULL != pb->core.retry_timer) {
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
}


MAYBE_INLINE int pbntf_enqueue_for_processing_sync(pubnub_t* pb)
{
    PUBNUB_UNUSED(pb);
    return 0;
}


MAYBE_INLINE int pbntf_requeue_for_processing_sync(pubnub_t* pb)
{
    PUBNUB_UNUSED(pb);

    return 0;
}


MAYBE_INLINE int pbntf_watch_out_events_sync(pubnub_t* pbp)
{
    PUBNUB_UNUSED(pbp);
    return 0;
}


MAYBE_INLINE int pbntf_watch_in_events_sync(pubnub_t* pbp)
{
    PUBNUB_UNUSED(pbp);
    return 0;
}


MAYBE_INLINE void pbnc_tr_cxt_state_reset_sync(pubnub_t* pb)
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


MAYBE_INLINE enum pubnub_res pubnub_last_result_sync(pubnub_t* pb)
{
    enum pubnub_res result;
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pbnc_fsm((pubnub_t*)pb);
    }
    result = pb->core.last_result;
    if (result != PNR_OK){
        pbnc_tr_cxt_state_reset_sync(pb);
    }
    pubnub_mutex_unlock(pb->monitor);

    return result;
}


#ifndef PUBNUB_NTF_RUNTIME_SELECTION

int pbntf_init(pubnub_t* pb)
{
    PUBNUB_UNUSED(pb);
    return pbntf_init_sync();
}


int pbntf_got_socket(pubnub_t* pb) 
{
    return pbntf_got_socket_sync(pb);
}


void pbntf_update_socket(pubnub_t* pb)
{
    pbntf_update_socket_sync(pb);
}


void pbntf_start_transaction_timer(pubnub_t* pb)
{
    pbntf_start_transaction_timer_sync(pb);
}


void pbntf_start_wait_connect_timer(pubnub_t* pb)
{
    pbntf_start_wait_connect_timer_sync(pb);
}


void pbntf_lost_socket(pubnub_t* pb)
{
    pbntf_lost_socket_sync(pb);
}


void pbntf_trans_outcome(pubnub_t* pb, enum pubnub_state state) 
{
    pbntf_trans_outcome_sync(pb, state);
}


int pbntf_enqueue_for_processing(pubnub_t* pb) {
    return pbntf_enqueue_for_processing_sync(pb);
}


int pbntf_requeue_for_processing(pubnub_t* pb) 
{
    return pbntf_requeue_for_processing_sync(pb);
}


int pbntf_watch_in_events(pubnub_t* pbp)
{
    return pbntf_watch_in_events_sync(pbp);
}


int pbntf_watch_out_events(pubnub_t* pbp) 
{
    return pbntf_watch_out_events_sync(pbp);
}


void pbnc_tr_cxt_state_reset(pubnub_t* pb) {
    return pbnc_tr_cxt_state_reset_sync(pb);
}


enum pubnub_res pubnub_last_result(pubnub_t* pb) {
    return pubnub_last_result_sync(pb);
}
#endif

EM_JS(void, call_alert, (), {
    console.log('test');
});

EM_ASYNC_JS(void, send_fetch_request, (const char* url, const char* method, const char* headers, const char* body, int timeout), {
    let timeoutId;
    const controller = new AbortController();
    
    console.log('send_fetch_request');
    console.log(UTF8ToString(url));
    console.log(UTF8ToString(method));
    console.log(UTF8ToString(headers));
    console.log(UTF8ToString(body));
   
    const requestTimeout = new Promise((_, reject) => {
        timeoutId = setTimeout(() => {
            clearTimeout(timeoutId);
            reject(new Error('Request timeout')); 
            controller.abort('Cancel because of timeout');
        }, timeout * 1000);
    });

    const request = new Request(UTF8ToString(url), {
        method: UTF8ToString(method),
        headers: JSON.parse(UTF8ToString(headers)),
        redirect: 'follow',
        body: UTF8ToString(body)
    });

    await Promise.race([
        fetch(request, {
            signal: controller.signal,
            credentials: 'omit',
            cache: 'no-cache'
        }).then((response) => {
            if (timeoutId) clearTimeout(timeoutId);
            return response;
        }),
        requestTimeout
    ]).then(response => {
        // Handle response
        console.log('Fetch completed:', response);
    }).catch(error => {
        console.error('Fetch error:', error);
    });
});

enum pubnub_res pubnub_await(pubnub_t* pb)
{
    pbmsref_t       t0;
    enum pubnub_res result;
    bool            stopped = false;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);

#ifdef PUBNUB_NTF_RUNTIME_SELECTION
    if (PNA_SYNC != pb->api_policy) {
        return PNR_INTERNAL_ERROR;
    }
#endif /* PUBNUB_NTF_RUNTIME_SELECTION */
    call_alert();
    
    emscripten_run_script("console.log('test')");
    
    // Get URL, method, headers and body from pb context
    const char* url = strcat("http://", strcat(pb->origin, pb->core.http_buf));
    const char* method = "POST";
    const char* headers = "{}";

    int i = 0;
    for (;;) {
        if (pb->core.http_buf[i] == '{' || pb->core.http_buf[i] == '\"' || i > 16000) {
            break;
        }
        i++;
    }

    const char* body = pb->core.http_buf + i;
    
   
    printf("url: %s\n", url);
    
    printf("body: %s\n", body);

    // Call the send_fetch_request function with the parameters
    send_fetch_request(url, method, headers, body, 10000);
    
    /*t0 = pbms_start();
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
        pbnc_tr_cxt_state_reset_sync(pb);
    }*/

    pb->state = PBS_IDLE;
    pb->core.http_buf_len = 0;
    pb->core.http_reply = "";
    pb->core.http_content_len = 0;
    pb->core.http_buf_len = 0;
    pb->core.http_reply = "";
    pb->core.http_content_len = 0;
    pb->core.http_buf_len = 0;
    pubnub_mutex_unlock(pb->monitor);
    return result;
}


