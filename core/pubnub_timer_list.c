/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_timer_list.h"

#include "pubnub_internal.h"
#include "pubnub_assert.h"


void pubnub_timer_list_init(pubnub_t *pbp)
{
    pbp->previous = pbp->next = NULL;
}


pubnub_t *pubnub_timer_list_add(pubnub_t *list, pubnub_t *to_add)
{
    int timeout_to_add_ms = to_add->transaction_timeout_ms;
    pubnub_t *pbp;

    PUBNUB_ASSERT_OPT(to_add != NULL);

    if (NULL == list) {
        list = to_add;
        to_add->previous = to_add->next = NULL;
        to_add->timeout_left_ms = timeout_to_add_ms;
        return list;
    }

    PUBNUB_ASSERT_OPT(list != to_add);
    if (timeout_to_add_ms < list->timeout_left_ms) {
        list->timeout_left_ms -= timeout_to_add_ms;
        to_add->next = list;
        to_add->previous = NULL;
        list->previous = to_add;
        to_add->timeout_left_ms = timeout_to_add_ms;
        return to_add;
    }

    pbp = list;
    while (timeout_to_add_ms >= pbp->timeout_left_ms) {
        timeout_to_add_ms -= pbp->timeout_left_ms;
        if (NULL == pbp->next) {
            pbp->next = to_add;
            to_add->previous = pbp;
            to_add->next = NULL;
            to_add->timeout_left_ms = timeout_to_add_ms;
            return list;
        }
        pbp = pbp->next;
        PUBNUB_ASSERT_OPT(pbp != to_add);
    }

    pbp->timeout_left_ms -= timeout_to_add_ms;
    to_add->next = pbp;
    to_add->previous = pbp->previous;
    PUBNUB_ASSERT(NULL != pbp->previous);
    pbp->previous->next = to_add;

    pbp->previous = to_add;
    to_add->timeout_left_ms = timeout_to_add_ms;

    return list;
}


pubnub_t *pubnub_timer_list_remove(pubnub_t *list, pubnub_t *to_remove)
{
    PUBNUB_ASSERT_OPT(list != NULL);
    PUBNUB_ASSERT_OPT(to_remove != NULL);

    if (list == to_remove) {
        list = list->next;
        if (NULL != list) {
            list->previous = NULL;
        }
        to_remove->previous = to_remove->next = NULL;
        return list;
    }

    if (NULL == to_remove->next) {
        to_remove->previous->next = NULL;
        to_remove->previous = NULL;
        return list;
    }

    to_remove->next->timeout_left_ms += to_remove->timeout_left_ms;
    to_remove->previous->next = to_remove->next;
    to_remove->next->previous = to_remove->previous;
    to_remove->previous = to_remove->next = NULL;

    return list;
}


pubnub_t *pubnub_timer_list_as_time_goes_by(pubnub_t **pplist, int time_passed_ms)
{
    pubnub_t *list;
    pubnub_t *expired_list = NULL;

    PUBNUB_ASSERT_OPT(pplist != NULL);
    PUBNUB_ASSERT_OPT(time_passed_ms > 0);

    list = *pplist;
    if (NULL == list) {
        return NULL;
    }
    if (list->timeout_left_ms > time_passed_ms) {
        list->timeout_left_ms -= time_passed_ms;
        return NULL;
    }

    expired_list = list;
    while (list->timeout_left_ms <= time_passed_ms) {
        time_passed_ms -= list->timeout_left_ms;
        if (NULL == list->next) {
            *pplist = NULL;
            return expired_list;
        }
        list = list->next;
    }

    list->timeout_left_ms -= time_passed_ms;
    PUBNUB_ASSERT(list->previous != NULL);
    list->previous->next = NULL;
    list->previous = NULL;
    *pplist = list;

    return expired_list;
}


pubnub_t *pubnub_timer_list_next(pubnub_t *p)
{
    PUBNUB_ASSERT_OPT(p != NULL);
    return p->next;
}


pubnub_t *pubnub_timer_list_previous(pubnub_t *p)
{
    PUBNUB_ASSERT_OPT(p != NULL);
    return p->previous;
}
