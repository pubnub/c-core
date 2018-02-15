/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PBPAL_NTF_CALLBACK_POLLER
#define      INC_PBPAL_NTF_CALLBACK_POLLER


/** @file pubnub_ntf_callback_poller.h

    This is the interface of a "poller". A poller is a "group checker"
    for statuses of sockets or other connection handles (let's call
    them sockets for short, because that's the mainstream). We can add
    and remove sockets from this group, change what to "watch for" and such.
    Of course, we can actually, well, poll - get the current status of
    what sockets are "ready".

    The administration/book-keeping part of this is somewhat generic,
    though there are simple differences to handle.  Tut the "poll"
    itself is not. Even the mainstream has many variants of these -
    the BSD sockets themselves have poll() and select(), Unices in
    general have signal based I/O (with various degrees of usability
    issues), Linux has also epoll(), BSD Unices have kqueue, some
    other have `/dev/poll`... Windows has I/O completion ports and the
    older and seemingly "out of fashion" completion callbacks w/APC
    (Async Procedure Calls) - also Windows poll() is slightly
    different and is called WSAPoll().

    Of course, other networking APIs have similar, yet "different
    enough" interfaces.

    So, it's a jungle, really, but, they all basically provide the same
    interface and here we have it's definition for our purpose.
 */


struct pbpal_poll_data;
typedef struct pubnub_ pubnub_t;


/** Allocate and Initialize the poller data */
struct pbpal_poll_data* pbpal_ntf_callback_poller_init(void);

/** Add the Pubnub context @p pb to the poll-set @p data */
void pbpal_ntf_callback_save_socket(struct pbpal_poll_data* data, pubnub_t* pb);

/** Remove the Pubnub context @p pb from the poll-set @p data */
void pbpal_ntf_callback_remove_socket(struct pbpal_poll_data* data, pubnub_t* pb);

/** Update the information about the Pubnub context @p pb int the
    poll-set @p data. Essentially, this is used when the socket
    (connection handle) changes, for some reason.
*/
void pbpal_ntf_callback_update_socket(struct pbpal_poll_data* data, pubnub_t* pb);

/** Watch for "out" events ("can write") on @p pbp context in poll-set
    @p data.
 */
int pbpal_ntf_watch_out_events(struct pbpal_poll_data* data, pubnub_t* pbp);

/** Watch for "in" events ("can read") on @p pbp context in poll-set
    @p data.
 */
int pbpal_ntf_watch_in_events(struct pbpal_poll_data* data, pubnub_t* pbp);

/** Do the polling and queue any events to process. 

    Maybe it could return (give back) the contexts that need
    processing instead, but, if there are many, that would slow things
    down... Another option would be to pass a function pointer here
    that this function would call for each context that needs
    processing - but, we basically know what we want to do and it's
    not configurable.
 */
int pbpal_ntf_poll_away(struct pbpal_poll_data* data, int ms);

/** Deinitialize and deellocate the poller data */
void pbpal_ntf_callback_poller_deinit(struct pbpal_poll_data** data);


#endif  /* !defined INC_PBPAL_NTF_CALLBACK_POLLER */
