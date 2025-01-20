#include "core/pubnub_ntf_callback.h"
#include "pubnub_internal.h"
#include "core/pubnub_ntf_dynamic.h"
#include "core/pubnub_internal_common.h"
#include "pubnub_config.h"


enum determined_api_policy {
    API_SYNC,
    API_CALLBACK
};


static enum determined_api_policy determine_api_policy(pubnub_t* pb)
{
    switch (pb->api_policy) {
        case PNA_DYNAMIC:
            return (pb->cb != NULL) ? API_CALLBACK : API_SYNC;
        case PNA_SYNC:
            return API_SYNC;
        case PNA_CALLBACK:
            return API_CALLBACK;
    };
}


enum pubnub_res pubnub_enforce_api(pubnub_t* p, enum pubnub_api_enforcement policy) {
    enum pubnub_res rslt = PNR_OK;

    pubnub_mutex_lock(p->monitor);

    if (p->api_policy == policy) {
        pubnub_mutex_unlock(p->monitor);
        return PNR_OK;
    }

    // We shouldn't change the API policy if the transaction is already in progress
    if (!pbnc_can_start_transaction(p)) {
        pubnub_mutex_unlock(p->monitor);
        return PNR_IN_PROGRESS;
    }

    enum determined_api_policy previous_policy = determine_api_policy(p);

    p->api_policy = policy;

    enum determined_api_policy current_policy = determine_api_policy(p);

    if (previous_policy != current_policy) {
        switch (previous_policy) {
            case API_SYNC:
                if (0 != pbntf_init_callback()) {
                    rslt = PNR_INTERNAL_ERROR;
                }
                break;
            case API_CALLBACK:
                pubnub_stop();
                if (0 != pbntf_init_sync()) {
                    rslt = PNR_INTERNAL_ERROR;
                }
                break;
        }
    }

    pubnub_mutex_unlock(p->monitor);
    return rslt;
}


int pbntf_watch_in_events(pubnub_t* pbp)
{
    enum determined_api_policy api_policy = determine_api_policy(pbp);

    switch (api_policy) {
        case API_SYNC:
            return pbntf_watch_in_events_sync(pbp);
        case API_CALLBACK:
            return pbntf_watch_in_events_callback(pbp);
    }
}


int pbntf_watch_out_events(pubnub_t* pbp)
{
    enum determined_api_policy api_policy = determine_api_policy(pbp);

    switch (api_policy) {
        case API_SYNC:
            return pbntf_watch_out_events_sync(pbp);
        case API_CALLBACK:
            return pbntf_watch_out_events_callback(pbp);
    }
}

 
int pbntf_init(void)
{
    // Calling the sync version by default because in most of cases the sync version
    // would be selected as there won't be any callback set yet.
    // During the first callback addition, function pbntf_init_callback() will be called.
    pbntf_init_sync();
}


int pbntf_enqueue_for_processing(pubnub_t* pb)
{
    enum determined_api_policy api_policy = determine_api_policy(pb);

    switch (api_policy) {
        case API_SYNC:
            return pbntf_enqueue_for_processing_sync(pb);
        case API_CALLBACK:
            return pbntf_enqueue_for_processing_callback(pb);
    }
}


int pbntf_requeue_for_processing(pubnub_t* pb)
{
    enum determined_api_policy api_policy = determine_api_policy(pb);

    switch (api_policy) {
        case API_SYNC:
            return pbntf_requeue_for_processing_sync(pb);
        case API_CALLBACK:
            return pbntf_requeue_for_processing_callback(pb);
    }
}


int pbntf_got_socket(pubnub_t* pb) 
{
    enum determined_api_policy api_policy = determine_api_policy(pb);

    switch (api_policy) {
        case API_SYNC:
            return pbntf_got_socket_sync(pb);
        case API_CALLBACK:
            return pbntf_got_socket_callback(pb);
    }
}


void pbntf_lost_socket(pubnub_t* pb)
{
    enum determined_api_policy api_policy = determine_api_policy(pb);

    switch (api_policy) {
        case API_SYNC:
            pbntf_lost_socket_sync(pb);
            break;
        case API_CALLBACK:
            pbntf_lost_socket_callback(pb);
            break;
    }
}


void pbntf_start_wait_connect_timer(pubnub_t* pb)
{
    enum determined_api_policy api_policy = determine_api_policy(pb);

    switch (api_policy) {
        case API_SYNC:
            pbntf_start_wait_connect_timer_sync(pb);
            break;
        case API_CALLBACK:
            pbntf_start_wait_connect_timer_callback(pb);
            break;
    }
}


void pbntf_start_transaction_timer(pubnub_t* pb) 
{
    enum determined_api_policy api_policy = determine_api_policy(pb);

    switch (api_policy) {
        case API_SYNC:
            pbntf_start_transaction_timer_sync(pb);
            break;
        case API_CALLBACK:
            pbntf_start_transaction_timer_callback(pb);
            break;
    }
}


void pbntf_update_socket(pubnub_t* pb)
{
    enum determined_api_policy api_policy = determine_api_policy(pb);

    switch (api_policy) {
        case API_SYNC:
            pbntf_update_socket_sync(pb);
            break;
        case API_CALLBACK:
            pbntf_update_socket_callback(pb);
            break;
    }
}


