/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_TIMERS_IO
#define INC_PUBNUB_TIMERS_IO


#include "pubnub_api_types.h"
#include "pubnub_ccore_limits.h"
#include "lib/pb_extern.h"


/** @file pubnub_timers.h
    This is the "Timer" API of the Pubnub client library, pertaining
    to transaction timeout(s), that is the time limit on how long
    we allow a transaction to last before it finishes.

    Functions here influence the way that Pubnub client library works
    with lower levels (the TCP/IP stack) with respect to how long will
    an operation be waited upon to complete.

    It is available in most platforms, but the exact way it behaves
    may have noticable differences. In general, the actual timeout
    will be _at_ _least_ as the specified by the user, but, it may
    actually be more.
*/


/** The Pubnub default timeout for subscribe transaction, in milliseconds */
#define PUBNUB_DEFAULT_SUBSCRIBE_TIMEOUT PUBNUB_DEFAULT_TRANSACTION_TIMER

/** The Pubnub default timeout for non-subscribe transactions, in milliseconds
 */
#define PUBNUB_DEFAULT_NON_SUBSCRIBE_TIMEOUT PUBNUB_MIN_TRANSACTION_TIMER


/** Sets the transaction timeout for the context. This will be used
    for all subsequent transactions. If a transactions is ongoing and
    its timeout can be changed, it will be changed, but if it can't,
    that would not be reported as an error.

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

    @retval 0 OK
    @retval -1 timers not supported
*/
PUBNUB_EXTERN int pubnub_set_transaction_timeout(pubnub_t* p, int duration_ms);

/** Returns the current transaction timeout for the context.

    @pre Call this after pubnub_init() on the context
    @param p The Context to get the transaction timeout for
    @return Current transaction timeout, in milliseconds (should
    always be > 0)
*/
PUBNUB_EXTERN int pubnub_transaction_timeout_get(pubnub_t* p);


/** Sets the 'wait_connect_TCP_socket' timeout for the context.
    This will be used for all subsequent transactions. If a transaction
    is ongoing and its 'wait_connect' timeout can be changed, it will be
    changed, but if it can't, that would not be reported as an error.

    If timer support is available, pubnub_init() will set a default
    'wait_connect_TCP_socket' timeout, which is configurable at compile
    time. So, if the default 'wait_connect' timeout is fine with you,
    you don't have to call this function.

    @pre Call this after pubnub_init() on the context
    @pre duration_ms > 0
    @param p The Context to set 'wait_connect_TCP_socket' timeout for
    @param duration_ms Duration of the timeout, in milliseconds

    @retval 0 OK
    @retval -1 timers not supported
*/
PUBNUB_EXTERN int pubnub_set_wait_connect_timeout(pubnub_t* p, int duration_ms);

/** Returns the current 'wait_connect_TCP_socket' timeout for the context.

    @pre Call this after pubnub_init() on the context
    @param p The Context to get the 'wait_connect_TCP_socket' timeout for
    @return Current transaction timeout, in milliseconds (should
    always be > 0)
*/
PUBNUB_EXTERN int pubnub_wait_connect_timeout_get(pubnub_t* p);


#endif /* defined INC_PUBNUB_TIMERS_IO */
