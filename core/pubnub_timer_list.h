/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_TIMER_LIST
#define	INC_PUBNUB_TIMER_LIST


#include "pubnub_api_types.h"

/** Initialize the element of the list - so that it is not
    in the list.
    @pre pbp != NULL
    */
void pubnub_timer_list_init(pubnub_t *pbp);

/** Add/enqueue a Pubnub context into a list of contexts/timers.
    If it gets enqueued to the start of the list, caller needs
    to remember (i.e. can't ignore) the return value.
    
    @pre to_add != NULL
    @pre list != to_add
    @param list Pointer to the head of the list (can be NULL)
    @param to_add Context to add
    @return The head of the list (may differ from @p list)
 */
pubnub_t *pubnub_timer_list_add(pubnub_t *list, pubnub_t *to_add);

/** Remove/dequeue a Pubnub context from a list of contexts/timers.
    It is assumed (i.e. it is a precondition) that @p to_remove
    is in @p list.
    
    @pre list != NULL
    @pre to_remove != NULL
    @param list Pointer to the head of the list
    @param to_remove Context to remove from the @p list
    @return The head of the list (may differ from @p list)
 */
pubnub_t *pubnub_timer_list_remove(pubnub_t *list, pubnub_t *to_remove);

/** Examines the @p list of timers/contexts, taking into account that
    @p time_passed_ms has gone by since last check, and dequeues all
    that have expired in that time, returning them in a list.

    @pre list != NULL
    @pre time_passed_ms > 0
    @param pplist Pointer to the pointer to the header of the list to examine
    (will change the header pointer if some timer(s) have expired)
    @param time_passed_ms Number of milliseconds passed since last check
    @return List of expired timers (NULL if none have expired)
 */
pubnub_t *pubnub_timer_list_as_time_goes_by(pubnub_t **pplist, int time_passed_ms);

/** Returns a pointer to the next element in the list for the given
    element @p p.
    @pre p != NULL
    */
pubnub_t *pubnub_timer_list_next(pubnub_t *p);

/** Returns a pointer to the previous element in the list for the given
    element @p p.
    @pre p != NULL
    */
pubnub_t *pubnub_timer_list_previous(pubnub_t *p);


#endif /* !defined INC_PUBNUB_TIMER_LIST */
