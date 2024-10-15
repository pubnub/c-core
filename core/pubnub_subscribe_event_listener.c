/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_subscribe_event_listener.h"


#include "pbcc_memory_utils.h"
#include "core/pubnub_subscribe_event_engine_internal.h"
#include "core/pubnub_log.h"
#include "pubnub_internal.h"
#include "lib/pbstrdup.h"


// ----------------------------------------------
//             Function prototypes
// ----------------------------------------------

/**
 * @brief Update registered list of listeners.
 *
 * @param event_listener Pointer to the Event Listener for which registered
 *                       listeners should be updated.
 * @param add            Whether listeners registered or unregistered.
 * @param type           Type of real-time update for which listener will be
 *                       called.
 * @param subscription   Pointer to the subscription object for which 'callback'
 *                       for specific real-time updates 'type' will be
 *                       registered.
 * @param subscribables  Pointer to the subscription object subscribables.
 * @param callback       Real-time update handling listener function.
 * @return Result of listeners list update operation.
 */
static enum pubnub_res pubnub_subscribe_manage_listener_(
    pbcc_event_listener_t*              event_listener,
    bool                                add,
    pubnub_subscribe_listener_type      type,
    const void*                         subscription,
    pbhash_set_t*                       subscribables,
    pubnub_subscribe_message_callback_t callback);

/**
 * @brief Get list of channel and groups which can be used to map listeners.
 *
 * @param subscribables Pointer to the set of subscribables for which names of
 *                      channels and groups should be retrieved.
 * @return Pointer to the list of subscribable names or `NULL` in case of
 *         insufficient memory error. The returned pointer must be passed to the
 *         `pbarray_free` to avoid a memory leak.
 */
static pbarray_t* pubnub_subscribe_subscribable_names_(
    pbhash_set_t* subscribables);


// ----------------------------------------------
//                   Functions
// ----------------------------------------------

enum pubnub_res pubnub_subscribe_add_status_listener(
    const pubnub_t*                          pb,
    const pubnub_subscribe_status_callback_t callback)
{
    pbcc_event_listener_t* event_listener =
        pbcc_subscribe_ee_event_listener(pb->core.subscribe_ee);
    return pbcc_event_listener_add_status_listener(event_listener, callback);
}

enum pubnub_res pubnub_subscribe_remove_status_listener(
    const pubnub_t*                          pb,
    const pubnub_subscribe_status_callback_t callback)
{
    pbcc_event_listener_t* event_listener =
        pbcc_subscribe_ee_event_listener(pb->core.subscribe_ee);
    return pbcc_event_listener_remove_status_listener(event_listener, callback);
}

enum pubnub_res pubnub_subscribe_add_message_listener(
    const pubnub_t*                           pb,
    const pubnub_subscribe_listener_type      type,
    const pubnub_subscribe_message_callback_t callback)
{
    pbcc_event_listener_t* event_listener =
        pbcc_subscribe_ee_event_listener(pb->core.subscribe_ee);
    return pbcc_event_listener_add_message_listener(
        event_listener,
        type,
        callback);
}

enum pubnub_res pubnub_subscribe_remove_message_listener(
    const pubnub_t*                           pb,
    const pubnub_subscribe_listener_type      type,
    const pubnub_subscribe_message_callback_t callback)
{
    pbcc_event_listener_t* event_listener =
        pbcc_subscribe_ee_event_listener(pb->core.subscribe_ee);
    return pbcc_event_listener_remove_message_listener(
        event_listener,
        type,
        callback);
}

enum pubnub_res pubnub_subscribe_add_subscription_listener(
    const pubnub_subscription_t*              subscription,
    const pubnub_subscribe_listener_type      type,
    const pubnub_subscribe_message_callback_t callback)
{
    pbhash_set_t* subs = pubnub_subscription_subscribables_(subscription, NULL);
    if (NULL == subs) { return PNR_OUT_OF_MEMORY; }

    pbcc_event_listener_t* event_listener =
        pbcc_subscribe_ee_event_listener(subscription->ee);
    const enum pubnub_res rslt = pubnub_subscribe_manage_listener_(
        event_listener,
        true,
        type,
        subscription,
        subs,
        callback);
    pbhash_set_free_with_destructor(
        &subs,
        (pbhash_set_element_free)pubnub_subscribable_free_);

    return rslt;
}

enum pubnub_res pubnub_subscribe_remove_subscription_listener(
    const pubnub_subscription_t*              subscription,
    const pubnub_subscribe_listener_type      type,
    const pubnub_subscribe_message_callback_t callback)
{
    pbhash_set_t* subs = pubnub_subscription_subscribables_(subscription, NULL);
    if (NULL == subs) { return PNR_OUT_OF_MEMORY; }

    pbcc_event_listener_t* event_listener =
        pbcc_subscribe_ee_event_listener(subscription->ee);
    const enum pubnub_res rslt = pubnub_subscribe_manage_listener_(
        event_listener,
        false,
        type,
        subscription,
        subs,
        callback);
    pbhash_set_free_with_destructor(
        &subs,
        (pbhash_set_element_free)pubnub_subscribable_free_);

    return rslt;
}

enum pubnub_res pubnub_subscribe_add_subscription_set_listener(
    const pubnub_subscription_set_t*          subscription_set,
    const pubnub_subscribe_listener_type      type,
    const pubnub_subscribe_message_callback_t callback)
{
    pbhash_set_t* subs =
        pubnub_subscription_set_subscribables_(subscription_set);
    if (NULL == subs) { return PNR_OUT_OF_MEMORY; }

    pbcc_event_listener_t* event_listener =
        pbcc_subscribe_ee_event_listener(subscription_set->ee);
    const enum pubnub_res rslt = pubnub_subscribe_manage_listener_(
        event_listener,
        true,
        type,
        subscription_set,
        subs,
        callback);
    pbhash_set_free_with_destructor(
        &subs,
        (pbhash_set_element_free)pubnub_subscribable_free_);

    return rslt;
}

enum pubnub_res pubnub_subscribe_remove_subscription_set_listener(
    const pubnub_subscription_set_t*          subscription_set,
    const pubnub_subscribe_listener_type      type,
    const pubnub_subscribe_message_callback_t callback)
{
    pbhash_set_t* subs =
        pubnub_subscription_set_subscribables_(subscription_set);
    if (NULL == subs) { return PNR_OUT_OF_MEMORY; }

    pbcc_event_listener_t* event_listener =
        pbcc_subscribe_ee_event_listener(subscription_set->ee);
    const enum pubnub_res rslt = pubnub_subscribe_manage_listener_(
        event_listener,
        false,
        type,
        subscription_set,
        subs,
        callback);
    pbhash_set_free_with_destructor(
        &subs,
        (pbhash_set_element_free)pubnub_subscribable_free_);

    return rslt;
}

enum pubnub_res pubnub_subscribe_manage_listener_(
    pbcc_event_listener_t*                    event_listener,
    const bool                                add,
    const pubnub_subscribe_listener_type      type,
    const void*                               subscription,
    pbhash_set_t*                             subscribables,
    const pubnub_subscribe_message_callback_t callback)
{
    if (NULL == subscribables) { return PNR_OUT_OF_MEMORY; }

    /** Get list of names for which listeners should be updated. */
    pbarray_t* names = pubnub_subscribe_subscribable_names_(subscribables);
    if (NULL == names) { return PNR_OUT_OF_MEMORY; }

    enum pubnub_res rslt = PNR_OK;
    if (add) {
        rslt = pbcc_event_listener_add_subscription_object_listener(
            event_listener,
            type,
            names,
            subscription,
            callback);
    }
    else {
        rslt = pbcc_event_listener_remove_subscription_object_listener(
            event_listener,
            type,
            names,
            subscribables,
            callback);
    }

    /**
     * If listeners added successfully then names memory managed by event
     * listener.
     */
    if (PNR_OK != rslt || !add) { pbarray_free_with_destructor(&names, free); }
    else { pbarray_free(&names); }

    return rslt;
}

pbarray_t* pubnub_subscribe_subscribable_names_(pbhash_set_t* subscribables)
{
    size_t                  count;
    pubnub_subscribable_t** subs = (pubnub_subscribable_t**)
        pbhash_set_elements(subscribables, &count);
    if (NULL == subs) {
        PUBNUB_LOG_ERROR("pubnub_subscribe_subscribable_names: failed to "
            "allocate memory for subscribables list\n");
        return NULL;
    }

    pbarray_t* names = pbarray_alloc(count,
                                     PBARRAY_RESIZE_NONE,
                                     PBARRAY_GENERIC_CONTENT_TYPE,
                                     NULL);
    if (NULL == names) {
        PUBNUB_LOG_ERROR("pubnub_subscribe_subscribable_names: failed to "
            "allocate memory for subscribable names\n");
        free(subs);
        return NULL;
    }

    for (size_t i = 0; i < count; ++i) {
        const pubnub_subscribable_t* subscribable = subs[i];
        pbarray_res                  result = PBAR_OK;
        char*                        name = pbstrdup(subscribable->id->ptr);

        if (NULL != name) { result = pbarray_add(names, name); }
        if (NULL == name || PBAR_OK != result) {
            PUBNUB_LOG_ERROR("pubnub_subscribe_subscribable_names: failed to "
                "allocate memory for subscribable names array "
                "element\n");
            if (NULL != name) { free(name); }
            pbarray_free_with_destructor(&names, free);
            free(subs);
            return NULL;
        }
    }
    free(subs);

    return names;
}