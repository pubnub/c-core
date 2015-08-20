/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_ALLOC
#define      INC_PUBNUB_ALLOC


#include "pubnub_api_types.h"


/** Returns an allocated context. After successful allocation, please
    call pubnub_init() to prepare the context for regular use.

    Do not make a context on your own - always get a context pointer
    by calling this funciton.

    @return Context pointer on success, NULL on error
 */
pubnub_t *pubnub_alloc(void);

/** Frees a previously allocated context, if it is not in a
    transaction.  If a context is in a transaction, first cancel it
    (call pubnub_cancel()), then wait for the context to finish the
    cancelling.

    It's OK to call this function on a context whose transaction is
    not over and done, it will just fail, but will not affect the
    transaction in any way.

    You don't have to free a context when you finish a transaction.
    Just start a new one. Free a context if you're done doing Pubnub
    transactions for a long time.

    @param pb Pointer to a context which to free
    @return 0: OK, context freed; else: error, context untouched
*/
int pubnub_free(pubnub_t *pb);


#endif  /* !defined INC_PUBNUB_ALLOC */
