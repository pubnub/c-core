/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined(INC_PBPAL_NTF_CALLBACK_QUEUE)
#define INC_PBPAL_NTF_CALLBACK_QUEUE

#include "pubnub_mutex.h"


/** @file pbpal_ntf_callback_queue.h

    This is a helper module, has the common handling of the queue of
    events to be processed by a Pubnub context.

    In general, in an async/callback interface, we process all events
    on a context in the same thread (there could be more than one
    total thread, but one context should always be handled by the same
    thread). This module does the common stuff related to this queue,
    while other modules deal with platform-specific handling.

 */


#define PBPAL_NTF_CALLBACK_QUEUE_LIMIT 1024

/** The queue data. It's a simple circular buffer of (pointers to)
    contexts, with a mutex as a monitor for multithreading.
 */
struct pbpal_ntf_callback_queue {
    pubnub_mutex_t monitor;
    size_t         size;
    size_t         head;
    size_t         tail;
    pubnub_t*      apb[PBPAL_NTF_CALLBACK_QUEUE_LIMIT];
};


/** Initializes the @p queue */
void pbpal_ntf_callback_queue_init(struct pbpal_ntf_callback_queue* queue);


/** Deinitializes the @p queue */
void pbpal_ntf_callback_queue_deinit(struct pbpal_ntf_callback_queue* queue);


/** Enqueue Pubnub context @p pb for processing in the @p queue */
int pbpal_ntf_callback_enqueue_for_processing(struct pbpal_ntf_callback_queue* queue,
                                              pubnub_t* pb);


/** Requeue Pubnub context @p pb for processing in the @p queue. That
    is, if context is already in queue, let it be. If it's not,
    enqueue it.
 */
int pbpal_ntf_callback_requeue_for_processing(struct pbpal_ntf_callback_queue* queue,
                                              pubnub_t* pb);


/** Remove Pubnub context @p pb from @p queue */
void pbpal_ntf_callback_remove_from_queue(struct pbpal_ntf_callback_queue* queue,
                                          pubnub_t*                        pb);


/** Process all the context in the @p queue. */
void pbpal_ntf_callback_process_queue(struct pbpal_ntf_callback_queue* queue);


#endif /* !defined(INC_PBPAL_NTF_CALLBACK_QUEUE) */
