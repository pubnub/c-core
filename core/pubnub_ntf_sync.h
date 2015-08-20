/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_NTF_SYNC
#define	INC_PUBNUB_NTF_SYNC


#include "pubnub_api_types.h"


/** @file pubnub_ntf_sync.h 
    This is the "sync" notification interface.

    Pubnub client library offers two "mostly portable" interfaces for
    getting notification about the outcome of a Pubnub transaction.
    Those are:
    - sync (this one)
    - callback

    There are others, which are specific to a platform (like process
    events for Contiki OS). Also, not all platforms support the
    two "mostly portable" interfaces mentioned above.

    Sync interfaces is the simplest. You just check for the (last)
    result of a context (by using pubnub_last_result()) and when it
    becomes something other than #PNR_STARTED, you have your
    outcome. 

    Since pubnub_last_result() is part of the "Core API", this
    interface only provides one additional, helper function, which
    will wait in a loop for the transaction to finish.  While
    pubnub_last_result() is available in all Pubnub client libraries,
    pubnub_await() is not.
*/


/** Waits, in a loop, for the current transaction to finish. */
enum pubnub_res pubnub_await(pubnub_t *p);


#endif        /* defined INC_PUBNUB_NTF_CONTIKI */
