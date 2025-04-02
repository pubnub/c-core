#if defined PUBNUB_NTF_RUNTIME_SELECTION

#include "core/pubnub_assert.h"
#include "core/pubnub_ntf_callback.h"
#include "pubnub_log.h"
#include "pubnub_internal.h"
#include "core/pubnub_ntf_enforcement.h"
#include "core/pubnub_internal_common.h"
#include "pubnub_config.h"

void pubnub_enforce_api(pubnub_t* p, enum pubnub_api_enforcement policy) {
    PUBNUB_LOG_INFO("pubnub_enforce_api(pb=%p, policy=%d)\n", p, policy);
    p->api_policy = policy;
}


int pbntf_watch_in_events(pubnub_t* pbp)
{
    PUBNUB_LOG_TRACE("pbntf_watch_in_events(pb=%p, policy=%d)\n", pbp, pbp->api_policy);
    switch (pbp->api_policy) {
        case PNA_SYNC:
            return pbntf_watch_in_events_sync(pbp);
        case PNA_CALLBACK:
            return pbntf_watch_in_events_callback(pbp);
    }
}


int pbntf_watch_out_events(pubnub_t* pbp)
{
    PUBNUB_LOG_TRACE("pbntf_watch_out_events(pb=%p, policy=%d)\n", pbp, pbp->api_policy);
    switch (pbp->api_policy) {
        case PNA_SYNC:
            return pbntf_watch_out_events_sync(pbp);
        case PNA_CALLBACK:
            return pbntf_watch_out_events_callback(pbp);
    }
}

 
int pbntf_init(pubnub_t* pb)
{
    PUBNUB_LOG_TRACE("pbntf_init(pb=%p, policy=%d)\n", pb, pb->api_policy);
    switch (pb->api_policy) {
        case PNA_SYNC:
            return pbntf_init_sync();
        case PNA_CALLBACK:
            return pbntf_init_callback();
    }
}


int pbntf_enqueue_for_processing(pubnub_t* pb)
{
    PUBNUB_LOG_TRACE("pbntf_enqueue_for_processing(pb=%p, policy=%d)\n", pb, pb->api_policy);
    switch (pb->api_policy) {
        case PNA_SYNC:
            return pbntf_enqueue_for_processing_sync(pb);
        case PNA_CALLBACK:
            return pbntf_enqueue_for_processing_callback(pb);
    }
}


int pbntf_requeue_for_processing(pubnub_t* pb)
{
    PUBNUB_LOG_TRACE("pbntf_requeue_for_processing(pb=%p, policy=%d)\n", pb, pb->api_policy);
    switch (pb->api_policy) {
        case PNA_SYNC:
            return pbntf_requeue_for_processing_sync(pb);
        case PNA_CALLBACK:
            return pbntf_requeue_for_processing_callback(pb);
    }
}


int pbntf_got_socket(pubnub_t* pb) 
{
    PUBNUB_LOG_TRACE("pbntf_got_socket(pb=%p, policy=%d)\n", pb, pb->api_policy);
    switch (pb->api_policy) {
        case PNA_SYNC:
            return pbntf_got_socket_sync(pb);
        case PNA_CALLBACK:
            return pbntf_got_socket_callback(pb);
    }
}


void pbntf_lost_socket(pubnub_t* pb)
{
    PUBNUB_LOG_TRACE("pbntf_lost_socket(pb=%p, policy=%d)\n", pb, pb->api_policy);
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
    PUBNUB_LOG_TRACE("pbntf_start_wait_connect_timer(pb=%p, policy=%d)\n", pb, pb->api_policy);
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
    PUBNUB_LOG_TRACE("pbntf_start_transaction_timer(pb=%p, policy=%d)\n", pb, pb->api_policy);
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
    PUBNUB_LOG_TRACE("pbntf_update_socket(pb=%p, policy=%d)\n", pb, pb->api_policy);
    switch (pb->api_policy) {
        case PNA_SYNC:
            pbntf_update_socket_sync(pb);
            break;
        case PNA_CALLBACK:
            pbntf_update_socket_callback(pb);
            break;
    }
}

enum pubnub_res pubnub_last_result(pubnub_t* pb) {
    PUBNUB_LOG_TRACE("pubnub_last_result(pb=%p, policy=%d)\n", pb, pb->api_policy);
    switch (pb->api_policy) {
        case PNA_SYNC:
            return pubnub_last_result_sync(pb);
        case PNA_CALLBACK:
            return pubnub_last_result_callback(pb);
    }
}


#endif /* PUBNUB_NTF_RUNTIME_SELECTION */
