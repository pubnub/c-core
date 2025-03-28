#ifndef PUBNUB_NTF_ENFORCEMENT_H
#define PUBNUB_NTF_ENFORCEMENT_H

#if defined PUBNUB_NTF_RUNTIME_SELECTION

struct pubnub_;
typedef struct pubnub_ pubnub_t;

enum pubnub_res;

#include "core/pubnub_netcore.h"
#include "lib/pb_extern.h"

/** The PubNub API enforcement policy.
 * 
 * This is used to select the PubNub API to be used by the client at runtime.
 * The client can use the sync API, the callback API.
 *
 * By default it selects the sync API.
 *
 * Policy is required only if the client is built with both sync and callback 
 * APIs. If the client is built with only one API, the policy is not even compiled.
 */
enum pubnub_api_enforcement {
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
 * It is crucial to select the API right after the allocation of the context
 * but before the initialization to let the client know which API to use.
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
PUBNUB_EXTERN void pubnub_enforce_api(pubnub_t* pb, enum pubnub_api_enforcement policy);

/* This section declares the functions that are used when the api enforcement 
 * policy is set. They are bridge between the sync and callback interfaces.
 */

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

/* End of the section */

#endif // PUBNUB_NTF_RUNTIME_SELECTION
#endif // PUBNUB_NTF_ENFORCEMENT_H
