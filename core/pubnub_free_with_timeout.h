/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_FREE_WITH_TIMEOUT
#define	INC_PUBNUB_FREE_WITH_TIMEOUT


/** Tries pubnub_free() in a tight loop until either:
    - it succeeds
    - time specified in @p millisec elapses

    Essentially, it waits for the context to finish its current
    transaction and then frees it.

    This function is much more useful in the callback interface,
    especially after a pubnub_cancel().

    This function is _not_ useful at all in the sync interface if
    you're using only one thread with the @p pbp context.

    Also, if you want to do some other processing while waiting for
    the transaction to finish, don't use this function.

    @param[in] pbp The context which to free
    @param[in] millisec Max time to wait for freeing to succeed

    @retval 0 pubnub_free() succeeded
    @retval -1 failed to pubnub_free() in @p microsec
 */
int pubnub_free_with_timeout(pubnub_t* pbp, unsigned millisec);


#endif /* !defined INC_PUBNUB_FREE_WITH_TIMEOUT */
