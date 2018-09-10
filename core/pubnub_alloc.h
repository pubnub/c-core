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
    transaction.  If a context is in a transaction, it will cancel it
    (as if you called pubnub_cancel()), but there's no guarantee that
    the cancellation will be finished during this call - if it
    doesn't, await the PNR_CANCELLED outcome and then call this
    function again.

    You don't have to free a context when you finish a transaction.
    Just start a new transaction. Free a context if you're done doing
    Pubnub transactions for a long time.

    @param pb Pointer to a context which to free
    @return 0: OK, context freed; else: not freed, transaction cancel started
*/
int pubnub_free(pubnub_t *pb);


#endif  /* !defined INC_PUBNUB_ALLOC */
