/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_FREE_WITH_TIMEOUT
#define	INC_PUBNUB_FREE_WITH_TIMEOUT

#include "pubnub_api_types.h"
#include "lib/pb_extern.h"


/** Tries pubnub_free() in a tight loop until either:
    - it succeeds
    - time specified in @p millisec elapses

    Essentially, it waits for the context to finish the cancellation
    (which is implied with pubnub_free()) and then frees it.

    This function is more useful in the callback interface. In the
    sync interface, there's a reasonable chance that a pubnub_free()
    would finish even if a transaction is ongoing.

    Also, if you want to do some other processing while waiting for
    the transaction to finish, don't use this function.

    @param[in] pbp The context which to free
    @param[in] millisec Max time to wait for freeing to succeed,
    in milliseconds

    @retval 0 pubnub_free() succeeded
    @retval -1 failed to pubnub_free() in @p millisec
 */
PUBNUB_EXTERN int pubnub_free_with_timeout(pubnub_t* pbp, unsigned millisec);


#endif /* !defined INC_PUBNUB_FREE_WITH_TIMEOUT */
