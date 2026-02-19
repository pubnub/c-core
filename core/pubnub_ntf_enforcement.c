#if defined PUBNUB_NTF_RUNTIME_SELECTION

#include "core/pubnub_assert.h"
#include "core/pubnub_ntf_callback.h"
#if PUBNUB_USE_LOGGER
#include "pbcc_logger_manager.h"
#endif // PUBNUB_USE_LOGGER
#include "pubnub_internal.h"
#include "core/pubnub_ntf_enforcement.h"
#include "core/pubnub_internal_common.h"
#include "pubnub_config.h"

void pubnub_enforce_api(pubnub_t* p, enum pubnub_api_enforcement policy)
{
    PUBNUB_LOG_DEBUG(
        p, "Enforce %s API.", PNA_SYNC == policy ? "sync" : "callback");
    p->api_policy = policy;
}


int pbntf_watch_in_events(pubnub_t* pbp)
{
    PUBNUB_LOG_TRACE(
        pbp,
        "Watch 'in' events for %s API.",
        PNA_SYNC == pbp->api_policy ? "sync" : "callback");
    switch (pbp->api_policy) {
    case PNA_SYNC:
        return pbntf_watch_in_events_sync(pbp);
    case PNA_CALLBACK:
        return pbntf_watch_in_events_callback(pbp);
    }
}


int pbntf_watch_out_events(pubnub_t* pbp)
{
    PUBNUB_LOG_TRACE(
        pbp,
        "Watch 'out' events for %s API.",
        PNA_SYNC == pbp->api_policy ? "sync" : "callback");
    switch (pbp->api_policy) {
    case PNA_SYNC:
        return pbntf_watch_out_events_sync(pbp);
    case PNA_CALLBACK:
        return pbntf_watch_out_events_callback(pbp);
    }
}


int pbntf_init(pubnub_t* pb)
{
    PUBNUB_LOG_TRACE(
        pb,
        "Init NTF for %s API.",
        PNA_SYNC == pb->api_policy ? "sync" : "callback");
    switch (pb->api_policy) {
    case PNA_SYNC:
        return pbntf_init_sync();
    case PNA_CALLBACK:
        return pbntf_init_callback(pb);
    }
}


int pbntf_enqueue_for_processing(pubnub_t* pb)
{
    PUBNUB_LOG_TRACE(
        pb,
        "Enqueue context for processing with %s API.",
        PNA_SYNC == pb->api_policy ? "sync" : "callback");
    switch (pb->api_policy) {
    case PNA_SYNC:
        return pbntf_enqueue_for_processing_sync(pb);
    case PNA_CALLBACK:
        return pbntf_enqueue_for_processing_callback(pb);
    }
}


int pbntf_requeue_for_processing(pubnub_t* pb)
{
    PUBNUB_LOG_TRACE(
        pb,
        "Requeue context for processing with %s API.",
        PNA_SYNC == pb->api_policy ? "sync" : "callback");
    switch (pb->api_policy) {
    case PNA_SYNC:
        return pbntf_requeue_for_processing_sync(pb);
    case PNA_CALLBACK:
        return pbntf_requeue_for_processing_callback(pb);
    }
}


int pbntf_got_socket(pubnub_t* pb)
{
    PUBNUB_LOG_TRACE(
        pb,
        "Got socket for %s API.",
        PNA_SYNC == pb->api_policy ? "sync" : "callback");
    switch (pb->api_policy) {
    case PNA_SYNC:
        return pbntf_got_socket_sync(pb);
    case PNA_CALLBACK:
        return pbntf_got_socket_callback(pb);
    }
}


void pbntf_lost_socket(pubnub_t* pb)
{
    PUBNUB_LOG_TRACE(
        pb,
        "Lost socket for %s API.",
        PNA_SYNC == pb->api_policy ? "sync" : "callback");
    switch (pb->api_policy) {
    case PNA_SYNC:
        pbntf_lost_socket_sync(pb);
        break;
    case PNA_CALLBACK:
        pbntf_lost_socket_callback(pb);
        break;
    }
}


void pbntf_start_wait_connect_timer(pubnub_t* pb)
{
    PUBNUB_LOG_TRACE(
        pb,
        "Start wait connect timer for %s API.",
        PNA_SYNC == pb->api_policy ? "sync" : "callback");
    switch (pb->api_policy) {
    case PNA_SYNC:
        pbntf_start_wait_connect_timer_sync(pb);
        break;
    case PNA_CALLBACK:
        pbntf_start_wait_connect_timer_callback(pb);
        break;
    }
}


void pbntf_start_transaction_timer(pubnub_t* pb)
{
    PUBNUB_LOG_TRACE(
        pb,
        "Start transaction timer for %s API.",
        PNA_SYNC == pb->api_policy ? "sync" : "callback");
    switch (pb->api_policy) {
    case PNA_SYNC:
        pbntf_start_transaction_timer_sync(pb);
        break;
    case PNA_CALLBACK:
        pbntf_start_transaction_timer_callback(pb);
        break;
    }
}


void pbntf_update_socket(pubnub_t* pb)
{
    PUBNUB_LOG_TRACE(
        pb,
        "Update socket for %s API.",
        PNA_SYNC == pb->api_policy ? "sync" : "callback");
    switch (pb->api_policy) {
    case PNA_SYNC:
        pbntf_update_socket_sync(pb);
        break;
    case PNA_CALLBACK:
        pbntf_update_socket_callback(pb);
        break;
    }
}

enum pubnub_res pubnub_last_result(pubnub_t* pb)
{
    PUBNUB_LOG_TRACE(
        pb,
        "Get last result using %s API.",
        PNA_SYNC == pb->api_policy ? "sync" : "callback");
    switch (pb->api_policy) {
    case PNA_SYNC:
        return pubnub_last_result_sync(pb);
    case PNA_CALLBACK:
        return pubnub_last_result_callback(pb);
    }
}


#endif /* PUBNUB_NTF_RUNTIME_SELECTION */
