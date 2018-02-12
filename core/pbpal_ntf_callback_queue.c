/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#include "pbpal_ntf_callback_queue.h"

#include "pubnub_assert.h"


void pbpal_ntf_callback_queue_init(struct pbpal_ntf_callback_queue* queue)
{
    pubnub_mutex_init(queue->monitor);
    queue->size = sizeof queue->apb / sizeof queue->apb[0];
    queue->head = queue->tail = 0;
}


void pbpal_ntf_callback_queue_deinit(struct pbpal_ntf_callback_queue* queue)
{
    pubnub_mutex_destroy(queue->monitor);
    queue->head = queue->tail = 0;
}


int pbpal_ntf_callback_enqueue_for_processing(struct pbpal_ntf_callback_queue* queue,
                                              pubnub_t* pb)
{
    int    result;
    size_t next_head;

    PUBNUB_ASSERT_OPT(queue != NULL);
    PUBNUB_ASSERT_OPT(pb != NULL);

    pubnub_mutex_lock(queue->monitor);
    next_head = queue->head + 1;
    if (next_head == queue->size) {
        next_head = 0;
    }
    if (next_head != queue->tail) {
        queue->apb[queue->head] = pb;
        queue->head             = next_head;
        result                  = +1;
    }
    else {
        result = -1;
    }
    pubnub_mutex_unlock(queue->monitor);

    return result;
}


int pbpal_ntf_callback_requeue_for_processing(struct pbpal_ntf_callback_queue* queue,
                                              pubnub_t* pb)
{
    bool   found = false;
    size_t i;

    PUBNUB_ASSERT_OPT(pb != NULL);

    pubnub_mutex_lock(queue->monitor);
    for (i = queue->tail; i != queue->head;
         i = (((i + 1) == queue->size) ? 0 : i + 1)) {
        if (queue->apb[i] == pb) {
            found = true;
            break;
        }
    }
    pubnub_mutex_unlock(queue->monitor);

    return !found ? pbpal_ntf_callback_enqueue_for_processing(queue, pb) : 0;
}


void pbpal_ntf_callback_remove_from_queue(struct pbpal_ntf_callback_queue* queue,
                                          pubnub_t*                        pb)
{
    size_t i;

    PUBNUB_ASSERT_OPT(queue != NULL);
    PUBNUB_ASSERT_OPT(pb != NULL);

    pubnub_mutex_lock(queue->monitor);
    for (i = queue->tail; i != queue->head;
         i = (((i + 1) == queue->size) ? 0 : i + 1)) {
        if (queue->apb[i] == pb) {
            queue->apb[i] = NULL;
            break;
        }
    }
    pubnub_mutex_unlock(queue->monitor);
}


void pbpal_ntf_callback_process_queue(struct pbpal_ntf_callback_queue* queue)
{
    pubnub_mutex_lock(queue->monitor);
    while (queue->head != queue->tail) {
        pubnub_t* pbp = queue->apb[queue->tail++];
        if (queue->tail == queue->size) {
            queue->tail = 0;
        }
        if (pbp != NULL) {
            pubnub_mutex_unlock(queue->monitor);
            pubnub_mutex_lock(pbp->monitor);
            if (pbp->state == PBS_NULL) {
                pubnub_mutex_unlock(pbp->monitor);
                pballoc_free_at_last(pbp);
            }
            else {
                pbnc_fsm(pbp);
                pubnub_mutex_unlock(pbp->monitor);
            }
            pubnub_mutex_lock(queue->monitor);
        }
    }
    pubnub_mutex_unlock(queue->monitor);
}
