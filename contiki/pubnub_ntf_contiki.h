/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_NTF_CONTIKI
#define	INC_PUBNUB_NTF_CONTIKI

#include "contiki.h"


/** @file pubnub_ntf_contiki.h */

/** The ID of the Pubnub Publish event. Event carries the context
    pointer on which the publish transaction finished. Use
    pubnub_last_result() to read the outcome of the transaction.
 */
extern process_event_t pubnub_publish_event;

/** The ID of the Pubnub Subscribe event. Event carries the context
    pointer on which the subscribe transaction finished. Use
    pubnub_last_result() to read the outcome of the transaction.
 */
extern process_event_t pubnub_subscribe_event;

/** The ID of the Pubnub Time event. Event carries the context
    pointer on which the time transaction finished. Use
    pubnub_last_result() to read the outcome of the transaction.
 */
extern process_event_t pubnub_time_event;

/** The ID of the Pubnub History event. The same ID is used for
    History v1 and v2 events.

    Event carries the context pointer on which the history transaction
    finished. Use pubnub_last_result() to read the outcome of the
    transaction.
 */
extern process_event_t pubnub_history_event;

/** The ID of the Pubnub Prsence event. The same ID is used for
    all presence events: here now, global here now, where now, leave.

    Event carries the context pointer on which the history transaction
    finished. Use pubnub_last_result() to read the outcome of the
    transaction.
 */
extern process_event_t pubnub_history_event;


PROCESS_NAME(pubnub_process);


#endif        /* defined INC_PUBNUB_NTF_CONTIKI */
