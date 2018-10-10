/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_POSIX_CALLBACK
#define	INC_PUBNUB_POSIX_CALLBACK

/** @mainpage The POSIX Pubnub client library - callback interface

    This is the "callback" interface of the Pubnub client library for
    POSIX compatible OSes (Linux, BSD...). 

    User cannot make any assumptions about the thread on which
    the callback is called. 

    A common method to handle the callback is to simply notify the
    thread that initiated the transaction (via some message queue
    facility) and not do any processing in the callback itself. Of
    course, it is not always the right one.

    You can have multiple pubnub contexts established; in each
    context, at most one Pubnub API call/transaction may be ongoing
    (typically a "publish" or a "subscribe"). You can use more than
    one context in a thread, but, as mentioned above, should not use
    the same context from more than one thread.
    
 */

/** @file pubnub_contiki.h */

#include "pubnub_config.h"

/* -- You should not change anything below this line -- */

#if !defined(PUBNUB_CALLBACK_API)
#error You have to define PUBNUB_CALLBACK_API
#endif

#include "core/pubnub_alloc.h"
#include "core/pubnub_assert.h"
#include "core/pubnub_coreapi.h"
#include "core/pubnub_pubsubapi.h"
#include "core/pubnub_ntf_callback.h"
#include "core/pubnub_generate_uuid.h"
#if PUBNUB_USE_SUBSCRIBE_V2
#include "core/pubnub_subscribe_v2.h"
#endif


#endif /* !defined INC_PUBNUB_CALLBACK */
