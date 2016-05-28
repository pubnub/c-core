/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_FREERTOS_CALLBACK
#define	INC_PUBNUB_FREERTOS_CALLBACK

/** @mainpage The MPLAB Harmony Pubnub client library - callback interface

    This is the "callback" interface of the Pubnub client library for
    the MPLAB Harmony.

    The "callback" interface has these characteristics:
    - It is not thread safe. 
    - User cannot make any assumptions about the thread on which
    the callback is called. 

    In general, this should not be a problem, as Harmony applications are
    mostly not using threads. A future version will provide support for
    threads.

    So, she should synchronise access to the Pubnub context from the
    callback and the rest of the code. This can be done via some sync
    mechanism (mutex et al) or simply by not touching the context from
    the "initiator" thread after a transaction is initiated.

    You can have multiple pubnub contexts established; in each
    context, at most one Pubnub API call/transaction may be ongoing
    (typically a "publish" or a "subscribe"). You can use more than
    one context in a thread, but, as mentioned above, should not use
    the same context from more than one thread.
    
 */

/** @file pubnub_callback.h */

#include "pubnub_config.h"

/* -- You should not change anything below this line -- */

#if !defined(PUBNUB_CALLBACK_API)
#error You have to define PUBNUB_CALLBACK_API
#endif

#include "pubnub_alloc.h"
#include "pubnub_assert.h"
#include "pubnub_coreapi.h"
#include "pubnub_ntf_callback.h"
#include "pubnub_generate_uuid.h"

/* The Pubnub task to be called from the main loop of the application */
void pubnub_task(void);

/** For logging, we use the Harmony Console module */
#define PUBNUB_LOG_PRINTF   SYS_CONSOLE_PRINT


#endif /* !defined INC_PUBNUB_FREERTOS_CALLBACK */
