/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#include "pbpal_ntf_callback_handle_timer_list.h"

#include "pubnub_timer_list.h"
#include "pubnub_assert.h"
#include "pubnub_log.h"


void pbntf_handle_timer_list(int ms_elapsed, pubnub_t** head)
{
    pubnub_t* expired;

    PUBNUB_ASSERT_OPT(head != NULL);
    PUBNUB_ASSERT_OPT(ms_elapsed > 0);

    expired = pubnub_timer_list_as_time_goes_by(head, ms_elapsed);
    while (expired != NULL) {
        pubnub_t* next;

        pubnub_mutex_lock(expired->monitor);
        next = expired->next;
        pbnc_stop(expired, PNR_TIMEOUT);
        expired->next     = NULL;
        expired->previous = NULL;
        pubnub_mutex_unlock(expired->monitor);

        expired = next;
    }
}


void pbpal_remove_timer_safe(pubnub_t* to_remove, pubnub_t** from_head)
{
    PUBNUB_ASSERT_OPT(to_remove != NULL);
    PUBNUB_ASSERT_OPT(from_head != NULL);

    if (PUBNUB_TIMERS_API) {
        if ((to_remove->previous != NULL) || (to_remove->next != NULL)
            || (to_remove == *from_head)) {
            *from_head = pubnub_timer_list_remove(*from_head, to_remove);
        }
        else {
            PUBNUB_LOG_TRACE("pbpal_remove_timer_safe(to_remove=%p, *from_head=%p) don't remove:\n"
                             "to_remove->previous=%p, to_remove->next=%p\n",
                             to_remove, *from_head,
                             to_remove->previous, to_remove->next);
        }
    }
}
