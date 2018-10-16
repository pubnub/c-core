/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_FREERTOS_SYNC
#define	INC_PUBNUB_FREERTOS_SYNC

/** @mainpage The FreeRTOS Pubnub client library - sync interface

    This is the "sync" interface of the Pubnub client library for
    the FreeRTOS.

    The "sync" interface has these characteristics:
    - It is not thread safe. Please, always call all Pubnub functions
    on a context from the same thread (that is, same that allocated
    that context).
    - There is no "callback" on the end of a transaction. Instead,
    you should check (via pubnub_last_result()) to see if the 
    transaction completed, or if you don't need to do anything
    else on the thread until the transaction is completed (including
    possible thread shutdown), you can call pubnub_await() which
    will do it for you (and return the final result).

    You can have multiple pubnub contexts established; in each
    context, at most one Pubnub API call/transaction may be ongoing
    (typically a "publish" or a "subscribe"). You can use more than
    one context in a thread, but, as mentioned above, should not use
    the same context from more than one thread.
    
 */

/** @file pubnub_sync.h */

#include "pubnub_config.h"

/* -- You should not change anything below this line -- */

#include "pubnub_alloc.h"
#include "pubnub_assert.h"
#include "pubnub_pubsubapi.h"
#include "pubnub_coreapi.h"
#include "pubnub_ntf_sync.h"
#include "pubnub_generate_uuid.h"
#if PUBNUB_USE_SUBSCRIBE_V2
#include "core/pubnub_subscribe_v2.h"
#endif


#endif /* !defined INC_PUBNUB_FREERTOS_SYNC */
