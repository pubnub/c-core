#ifndef PUBNUB_NTF_DYNAMIC_H
#define PUBNUB_NTF_DYNAMIC_H

// TODO: FIX THIS FLAG MANAGMENT BEFORE MERGING
//#if PUBNUB_NTF_DYNAMIC

#include "core/pubnub_api_types.h"

/** The PubNub API enforcement policy.
 * 
 * This is used to select the PubNub API to be used by the client at runtime.
 * The client can use the sync API, the callback API, or it can
 * dynamically select the API depending on the context.
 *
 * By default API will be selected depending on the context.
 *
 * Policy is required only if the client is built with both sync and callback 
 * APIs. If the client is built with only one API, the policy is not even compiled.
 */
enum pubnub_api_enforcement {
    /** The PubNub API will be selected depending on the context.
     * By default, The client will use the sync API, unless any 
     * callback is set, in which case the client will use the callback API.
     */
    PNA_DYNAMIC,

    /** The PubNub client will enforce the sync API. */
    PNA_SYNC,

    /** The PubNub client will be the callback API. */
    PNA_CALLBACK
};


/** Enforce the PubNub API policy.
 *
 * This function is used to enforce the PubNub API policy.
 * The policy is used to select the PubNub API to be used by the client.
 *
 * Policy is required only if the client is built with both sync and callback 
 * APIs. If the client is built with only one API, the policy is not even compiled.
 *
 * @param pb The PubNub context.
 * @param policy The PubNub API enforcement policy.
 *
 * @return PNR_OK if the policy is enforced successfully or one of matching 
 * `pubnub_res` error codes.
 *
 * @see pubnub_api_enforcement
 * @see pubnub_res
 */
enum pubnub_res pubnub_enforce_api(pubnub_t* pb, enum pubnub_api_enforcement policy);


// TODO: maybe move it to pbcc file

void pbntf_trans_outcome_sync(pubnub_t* pb, enum pubnub_state state);
void pbntf_trans_outcome_callback(pubnub_t* pb, enum pubnub_state state);

int pbntf_init_sync(void);
int pbntf_init_callback(void);

int pbntf_got_socket_sync(pubnub_t* pb);
int pbntf_got_socket_callback(pubnub_t* pb);

void pbntf_update_socket_sync(pubnub_t* pb);
void pbntf_update_socket_callback(pubnub_t* pb);

void pbntf_start_wait_connect_timer_sync(pubnub_t* pb);
void pbntf_start_wait_connect_timer_callback(pubnub_t* pb);

void pbntf_start_transaction_timer_sync(pubnub_t* pb);
void pbntf_start_transaction_timer_callback(pubnub_t* pb);

void pbntf_lost_socket_sync(pubnub_t* pb);
void pbntf_lost_socket_callback(pubnub_t* pb);

int pbntf_enqueue_for_processing_sync(pubnub_t* pb);
int pbntf_enqueue_for_processing_callback(pubnub_t* pb);

int pbntf_requeue_for_processing_sync(pubnub_t* pb);
int pbntf_requeue_for_processing_callback(pubnub_t* pb);

int pbntf_watch_in_events_sync(pubnub_t* pb);
int pbntf_watch_in_events_callback(pubnub_t* pb);

int pbntf_watch_out_events_sync(pubnub_t* pb);
int pbntf_watch_out_events_callback(pubnub_t* pb);

void pbnc_tr_cxt_state_reset_sync(pubnub_t* pb);
void pbnc_tr_cxt_state_reset_callback(pubnub_t* pb);

enum pubnub_res pubnub_last_result_sync(pubnub_t* pb);
enum pubnub_res pubnub_last_result_callback(pubnub_t* pb);

//#endif // PUBNUB_NTF_DYNAMIC
#endif // PUBNUB_NTF_DYNAMIC_H
