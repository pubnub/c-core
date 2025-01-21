#if defined PUBNUB_NTF_RUNTIME_SELECTION

#include "core/pubnub_assert.h"
#include "core/pubnub_ntf_callback.h"
#include "pubnub_internal.h"
#include "core/pubnub_ntf_dynamic.h"
#include "core/pubnub_internal_common.h"
#include "pubnub_config.h"

void pubnub_enforce_api(pubnub_t* p, enum pubnub_api_enforcement policy) {
    p->api_policy = policy;
}


int pbntf_watch_in_events(pubnub_t* pbp)
{
    switch (pbp->api_policy) {
        case PNA_SYNC:
            return pbntf_watch_in_events_sync(pbp);
        case PNA_CALLBACK:
            return pbntf_watch_in_events_callback(pbp);
    }
}


int pbntf_watch_out_events(pubnub_t* pbp)
{
    switch (pbp->api_policy) {
        case PNA_SYNC:
            return pbntf_watch_out_events_sync(pbp);
        case PNA_CALLBACK:
            return pbntf_watch_out_events_callback(pbp);
    }
}

 
int pbntf_init(pubnub_t* pb)
{
    switch (pb->api_policy) {
        case PNA_SYNC:
            return pbntf_init_sync();
        case PNA_CALLBACK:
            return pbntf_init_callback();
    }
}


int pbntf_enqueue_for_processing(pubnub_t* pb)
{
    switch (pb->api_policy) {
        case PNA_SYNC:
            return pbntf_enqueue_for_processing_sync(pb);
        case PNA_CALLBACK:
            return pbntf_enqueue_for_processing_callback(pb);
    }
}


int pbntf_requeue_for_processing(pubnub_t* pb)
{
    switch (pb->api_policy) {
        case PNA_SYNC:
            return pbntf_requeue_for_processing_sync(pb);
        case PNA_CALLBACK:
            return pbntf_requeue_for_processing_callback(pb);
    }
}


int pbntf_got_socket(pubnub_t* pb) 
{
    switch (pb->api_policy) {
        case PNA_SYNC:
            return pbntf_got_socket_sync(pb);
        case PNA_CALLBACK:
            return pbntf_got_socket_callback(pb);
    }
}


void pbntf_lost_socket(pubnub_t* pb)
{
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
    switch (pb->api_policy) {
        case PNA_SYNC:
            pbntf_update_socket_sync(pb);
            break;
        case PNA_CALLBACK:
            pbntf_update_socket_callback(pb);
            break;
    }
}

#endif /* PUBNUB_NTF_RUNTIME_SELECTION */
