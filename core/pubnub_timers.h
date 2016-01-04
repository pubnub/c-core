/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_TIMERS_IO
#define	INC_PUBNUB_TIMERS_IO


#include "pubnub_api_types.h"


/** @file pubnub_timers.h 
    This is the "Timer" API of the Pubnub client library.
    Functions here influence the way that Pubnub client library
    works with lower levels (the TCP/IP stack) with respect to
    how much will an operation be waited upon to complete.

    It is available in most platforms, but the exact way it behaves
    may have noticable differences. In general, the actual timeout
    will be _at_ _least_ as the specified by the user, but, it may
    actually be more.
*/


/** The Pubnub default timeout on subscribe transaction, in milliseconds */
#define PUBNUB_DEFAULT_SUBSCRIBE_TIMEOUT 310000

/** The Pubnub default timeout on non-subscribe transactions, in milliseconds */
#define PUBNUB_DEFAULT_NON_SUBSCRIBE_TIMEOUT 10000


/** Sets the transaction timeout for the context. This will be
    used for all subsequent transactions. If a transactions is ongoing
    and its timeout can be changed, it will be, but if it can't, that 
    would not be reported as an error.
    
    Pubnub SDKs, in general, distinguish the "subscribe" timeout and
    other transactions, but, C-core doesn't, to save space, as most 
    contexts are either used for subscribe or for other transactions.
    
    If timer support is available, pubnub_init() will set a default timeout,
    which is configurable at compile time. So, if the default timeout is
    fine with you, you don't have to call this function.

    @pre Call this after pubnub_init() on the context
    @pre duration_ms > 0
    @param p The Context to set transaction timeout for
    @param duration_ms Duration of the timeout, in milliseconds

    @return 0: OK, otherwise: error, timers not supported  
*/
int pubnub_set_transaction_timeout(pubnub_t *p, int duration_ms);

/** Returns the current transaction timeout for the context.

    @pre Call this after pubnub_init() on the context
    @param p The Context for which to get the transaction timeout
    @return Current transaction timeout, in milliseconds (should
    always be > 0)
*/
int pubnub_transaction_timeout_get(pubnub_t *p);

#endif /* defined INC_PUBNUB_TIMERS_IO */
