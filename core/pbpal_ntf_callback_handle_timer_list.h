/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
typedef struct pubnub_ pubnub_t;

/** Checks the timer list with the given @p head for any expired
    timers, assuming that @p ms_elapsed since last check.

    For all expired, the context FSM will be called to handle
    the timeout.
 */
void pbntf_handle_timer_list(int ms_elapsed, pubnub_t** head);

/** Removes the context @p to_remove @p from_head list, in a "safe"
    manner. That is, it handles ("ignores") if @p to_remove is not in
    @p from_head.
 */
void pbpal_remove_timer_safe(pubnub_t* to_remove, pubnub_t** from_head);
