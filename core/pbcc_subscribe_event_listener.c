/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbcc_subscribe_event_listener.h"

#include <stdlib.h>

#include "core/pbcc_memory_utils.h"
#include "core/pubnub_mutex.h"
#include "lib/pbref_counter.h"
#include "core/pubnub_log.h"
#include "pubnub_internal.h"
#include "lib/pbhash_set.h"
#include "lib/pbarray.h"


// ----------------------------------------------
//                   Constants
// ----------------------------------------------

/** How many listener objects `pbcc_object_listener_t` can hold by default. */
#define LISTENERS_LENGTH 10


// ----------------------------------------------
//                    Types
// ----------------------------------------------

/**
 * @brief Channel / channel group listener definition.
 *
 * The object allows grouping listeners for the same subscribable entity and
 * calling them when update real-time arrives for the entity.
 */
typedef struct {
    /**
     * @brief Pointer to the byte string with the name of the channel / group
     *        from which listener expected to process updates.
     */
    char* name;
    /**
     * @brief Pointer to the array with real-time update listeners.
     *
     * @see pbcc_listener_t
     */
    pbarray_t* listeners;
} pbcc_object_listener_t;

/**
 * @brief Listener definition.
 *
 * Object listeners entry object with information about the source of real-time
 * updates and callback callback to use.
 */
typedef struct {
    /**
     * @brief Pointer to the subscription object which has been used to register
     *        listener.
     *
     * @note Value can be `NULL` if listener is stored for PubNub context.
     * @note Value needed only when subscription object listener removed from
     *       the list to better identify exact listener.
     *
     * @see pubnub_subscription_t
     * @see pubnub_subscription_set_t
     */
    const void* subscription_object;
    /** Type of real-time update for which listener will be called. */
    pubnub_subscribe_listener_type type;
    /** Real-time update handling listener function. */
    pubnub_subscribe_message_callback_t callback;
    /** Object references counter. */
    pbref_counter_t* counter;
} pbcc_listener_t;

/** Event Listener definition. */
struct pbcc_event_listener {
    /** Pointer to the array with subscription change listeners. */
    pbarray_t* global_status;
    /** Pointer to the array with PubNub context listeners (global). */
    pbarray_t* global_events;
    /**
     * @brief Pointer to the hash set with listeners mapped to the actual
     *        channel names.
     *
     * @see pbcc_object_listener_t
     */
    pbhash_set_t* listeners;
    /**
     * @brief Pointer to the PubNub context, which should be used with
     *        registered event listener functions.
     */
    const pubnub_t* pb;
    /** Shared resources access lock. */
    pubnub_mutex_t mutw;
};


// ----------------------------------------------
//             Function prototypes
// ----------------------------------------------

/**
 * @brief Create object-scoped list of real-time update listeners.
 *
 * @param name Pointer to the byte string with name of object for which update
 *             listeners will be registered.
 * @return Pointer to the ready to use updates listener or `NULL` in case of
 *         insufficient memory error. The returned pointer must be passed to the
 *         `_pbcc_object_listener_free` to avoid a memory leak.
 */
static pbcc_object_listener_t* pbcc_object_listener_alloc_(char* name);

/**
 * @brief Add / register object's listener.
 *
 * @param event_listener Pointer to the event listener which maintain objects'
 *                       real-time update listeners.
 * @param name           Pointer to the subscription name with which `listener`
 *                       should be associated.
 * @param listener       Pointer to the real-time update listener, which should
 *                       be used for new updates.
 * @return Result of the listener addition.
 */
static enum pubnub_res pbcc_add_object_listener_(
    const pbcc_event_listener_t* event_listener,
    char*                        name,
    pbcc_listener_t*             listener);

/**
* @brief Remove / unregister real-time update listener.
 *
 * @param event_listener Pointer to the event listener which maintain objects'
 *                       real-time update listeners.
 * @param name           Pointer to the subscription name with which `listener`
 *                       was associated.
 * @param listener       Pointer to the real-time update listener, which
 *                       shouldn't be used for updates anymore.
 * @return Result of the listener removal.
 */
static enum pubnub_res pbcc_remove_object_listener_(
    const pbcc_event_listener_t* event_listener,
    char*                        name,
    const pbcc_listener_t*       listener);

/**
 * @brief Clean up resources used by updates listener object.
 *
 * @param listener Pointer to the updates listener, which should free up
 *                         resources.
 */
static void pbcc_object_listener_free_(pbcc_object_listener_t* listener);

/**
 * @brief Create real-time events listener.
 *
 * @param subscription Pointer to the subscription object which has been used to
 *                     register listener.
 * @param type         Type of real-time update for which listener will be
 *                     called.
 * @param callback     Real-time update handling listener function.
 * @return Pointer to the ready to use real-time events listener or `NULL` in
 *         case of insufficient memory error. The returned pointer must be
 *         passed to the `_pbcc_listener_free` to avoid a memory leak.
 */
static pbcc_listener_t* pbcc_listener_alloc_(
    const void*                         subscription,
    pubnub_subscribe_listener_type      type,
    pubnub_subscribe_message_callback_t callback);

/**
 * @brief Add / register real-time update listener.
 *
 * @param listeners Pointer to the list of real-time update listeners.
 * @param listener  Pointer to the real-time update listener, which should be
 *                  used for new updates.
 * @return Result of the listener addition.
 */
static enum pubnub_res pbcc_add_listener_(
    pbarray_t*       listeners,
    pbcc_listener_t* listener);

/**
 * @brief Remove / unregister real-time update listener.
 *
 * @param listeners Pointer to the list of real-time update listeners.
 * @param listener  Pointer to the real-time update listener, which shouldn't be
 *                  used for updates anymore.
 * @return Result of the listener removal.
 */
static enum pubnub_res pbcc_remove_listener_(
    pbarray_t*             listeners,
    const pbcc_listener_t* listener);

/**
 * @brief Clean up resources used by the real-time updates listener object.
 *
 * @param listener Pointer to the object real-time events listeners, which
 *                 should free up resources.
 */
static void pbcc_listener_free_(pbcc_listener_t* listener);

/**
 * @brief Helper function to notify listeners from the list.
 *
 * Helper let iterate over the list of listeners and call them if the message
 * type corresponds to expected one or this is a global listener.
 *
 * @param listener  Pointer to the Event Listener, which contains provided list
 *                  of event listeners.
 * @param listeners Pointer to the array of event registered listeners which can
 *                  be notified about new `message`.
 * @param message   Received message which should be delivered to the listeners.
 */
static void pbcc_event_listener_emit_message_(
    const pbcc_event_listener_t* listener,
    pbarray_t*                   listeners,
    struct pubnub_v2_message     message);

/**
 * @brief Identify type of events source from the message type.
 *
 * Event Listener uses this information to identify listeners which explicitly
 * added to track specific event type.
 *
 * @param type Received message type specified by PubNub service.
 * @return Subscription event source type from message type.
 */
static pubnub_subscribe_listener_type pbcc_message_type_to_listener_type_(
    enum pubnub_message_type type);

/**
 * @brief Initialize and assign array.
 *
 * @param array   Pointer to the address of the array which should be
 *                initialized.
 * @param free_fn Array element destruction function. Can be set to `NULL` to
 *                keep elements after they removed from array or whole array is
 *                freed.
 * @return Pointer to the initialized array or `NULL` in case of insufficient
 *         memory error. The returned pointer must be passed to the
 *         `pbarray_free` to avoid a memory leak.
 */
static pbarray_t* pbcc_initialize_array_(
    pbarray_t**          array,
    pbarray_element_free free_fn);


// ----------------------------------------------
//                 Functions
// ----------------------------------------------

pbcc_event_listener_t* pbcc_event_listener_alloc(const pubnub_t* pb)
{
    PBCC_ALLOCATE_TYPE(listener, pbcc_event_listener_t, true, NULL);
    pubnub_mutex_init(listener->mutw);
    listener->global_events = NULL;
    listener->global_status = NULL;
    listener->listeners = NULL;
    listener->pb = pb;

    return listener;
}

enum pubnub_res pbcc_event_listener_add_status_listener(
    pbcc_event_listener_t*                   listener,
    const pubnub_subscribe_status_callback_t cb)
{
    if (NULL == listener || NULL == cb) { return PNR_INVALID_PARAMETERS; }

    pubnub_mutex_lock(listener->mutw);
    /** Check whether listeners array should be created on demand or not. */
    if (NULL == pbcc_initialize_array_(&listener->global_status, NULL)) {
        pubnub_mutex_unlock(listener->mutw);
        return PNR_OUT_OF_MEMORY;
    }

    const pbarray_res result = pbarray_add(listener->global_status, cb);
    pubnub_mutex_unlock(listener->mutw);

    return PBAR_OUT_OF_MEMORY != result ? PNR_OK : PNR_OUT_OF_MEMORY;
}

enum pubnub_res pbcc_event_listener_remove_status_listener(
    pbcc_event_listener_t*             listener,
    pubnub_subscribe_status_callback_t cb)
{
    if (NULL == listener || NULL == cb) { return PNR_INVALID_PARAMETERS; }

    pubnub_mutex_lock(listener->mutw);
    if (NULL == listener->global_status) {
        pubnub_mutex_unlock(listener->mutw);
        return PNR_OK;
    }

    pbarray_remove(listener->global_status, (void**)&cb, true);
    pubnub_mutex_unlock(listener->mutw);

    return PNR_OK;
}

enum pubnub_res pbcc_event_listener_add_message_listener(
    pbcc_event_listener_t*                    listener,
    const pubnub_subscribe_listener_type      type,
    const pubnub_subscribe_message_callback_t cb)
{
    if (NULL == listener || NULL == cb) { return PNR_INVALID_PARAMETERS; }

    pubnub_mutex_lock(listener->mutw);
    /** Check whether listeners array should be created on demand or not. */
    if (NULL == pbcc_initialize_array_(&listener->global_events,
                                       (pbarray_element_free)
                                       pbcc_listener_free_)) {
        pubnub_mutex_unlock(listener->mutw);
        return PNR_OUT_OF_MEMORY;
    }

    pbcc_listener_t* _listener = pbcc_listener_alloc_(NULL, type, cb);
    if (NULL == _listener) {
        pubnub_mutex_unlock(listener->mutw);
        return PNR_OUT_OF_MEMORY;
    }

    const enum pubnub_res result = pbcc_add_listener_(
        listener->global_events,
        _listener);
    if (PNR_OK != result) { pbcc_listener_free_(_listener); }
    pubnub_mutex_unlock(listener->mutw);

    return result;
}

enum pubnub_res pbcc_event_listener_remove_message_listener(
    pbcc_event_listener_t*                    listener,
    const pubnub_subscribe_listener_type      type,
    const pubnub_subscribe_message_callback_t cb)
{
    if (NULL == listener || NULL == cb)
        return PNR_INVALID_PARAMETERS;

    pubnub_mutex_lock(listener->mutw);
    if (NULL == listener->global_events) {
        pubnub_mutex_unlock(listener->mutw);
        return PNR_OK;
    }

    pbcc_listener_t* _listener = pbcc_listener_alloc_(NULL, type, cb);
    if (NULL == _listener) {
        pubnub_mutex_unlock(listener->mutw);
        return PNR_OUT_OF_MEMORY;
    }

    const enum pubnub_res rslt = pbcc_remove_listener_(
        listener->global_events,
        _listener);

    /**
     * It is safe to release temporarily object which used only to match object.
     */
    pbcc_listener_free_(_listener);
    pubnub_mutex_unlock(listener->mutw);

    return rslt;
}

enum pubnub_res pbcc_event_listener_add_subscription_object_listener(
    pbcc_event_listener_t*                    listener,
    const pubnub_subscribe_listener_type      type,
    pbarray_t*                                names,
    const void*                               subscription,
    const pubnub_subscribe_message_callback_t cb)
{
    if (NULL == listener || NULL == cb) { return PNR_INVALID_PARAMETERS; }

    pubnub_mutex_lock(listener->mutw);
    /** Check whether object listeners hash set is set or not. */
    if (NULL == listener->listeners) {
        listener->listeners = pbhash_set_alloc(
            LISTENERS_LENGTH,
            PBHASH_SET_CHAR_CONTENT_TYPE,
            (pbhash_set_element_free)pbcc_object_listener_free_);

        if (NULL == listener->listeners) {
            pubnub_mutex_unlock(listener->mutw);
            return PNR_OUT_OF_MEMORY;
        }
    }

    pbcc_listener_t* _listener = pbcc_listener_alloc_(subscription, type, cb);
    bool             added     = false;
    if (NULL == _listener) {
        pubnub_mutex_unlock(listener->mutw);
        return PNR_OUT_OF_MEMORY;
    }

    enum pubnub_res rslt        = PNR_OK;
    const size_t    names_count = pbarray_count(names);
    for (size_t i = 0; i < names_count; ++i) {
        char* name = (char*)pbarray_element_at(names, i);
        rslt       = pbcc_add_object_listener_(listener, name, _listener);
        if (PNR_OK == rslt) { added = true; }
        else { break; }
    }

    if (added && PNR_OK != rslt) {
        /** Remove any added entries of the `listener` in case of failure. */
        for (size_t i = 0; i < names_count; ++i) {
            char* name = (char*)pbarray_element_at(names, i);
            pbcc_remove_object_listener_(listener, name, _listener);
        }
    }
    /** Manual `free` required if arrays doesn't manage listener lifetime. */
    if (PNR_OK != rslt && !added) { pbcc_listener_free_(_listener); }
    pubnub_mutex_unlock(listener->mutw);

    return rslt;
}

enum pubnub_res pbcc_event_listener_remove_subscription_object_listener(
    pbcc_event_listener_t*                    listener,
    const pubnub_subscribe_listener_type      type,
    pbarray_t*                                names,
    const void*                               subscription,
    const pubnub_subscribe_message_callback_t cb)
{
    if (NULL == listener || NULL == cb) { return PNR_INVALID_PARAMETERS; }

    pubnub_mutex_lock(listener->mutw);
    if (NULL == listener->global_events) {
        pubnub_mutex_unlock(listener->mutw);
        return PNR_OK;
    }

    pbcc_listener_t* _listener = pbcc_listener_alloc_(subscription, type, cb);
    if (NULL == _listener) {
        pubnub_mutex_unlock(listener->mutw);
        return PNR_OUT_OF_MEMORY;
    }

    const size_t names_count = pbarray_count(names);
    for (size_t i = 0; i < names_count; ++i) {
        char* name = (char*)pbarray_element_at(names, i);
        pbcc_remove_object_listener_(listener, name, _listener);
    }

    /**
     * It is safe to release temporarily object which used only to match object.
     */
    pbcc_listener_free_(_listener);
    pubnub_mutex_unlock(listener->mutw);

    return PNR_OK;
}

void pbcc_event_listener_emit_status(
    pbcc_event_listener_t*           listener,
    const pubnub_subscription_status status,
    const enum pubnub_res            reason,
    const char*                      channels,
    const char*                      channel_groups)
{
    if (NULL == listener) { return; }

    pubnub_mutex_lock(listener->mutw);
    const pubnub_subscription_status_data_t data =
        { reason, channels, channel_groups };
    const size_t status_count = pbarray_count(listener->global_status);
    for (size_t i = 0; i < status_count; ++i) {
        const pubnub_subscribe_status_callback_t cb = (
                pubnub_subscribe_status_callback_t)
            pbarray_element_at(listener->global_status, i);
        cb(listener->pb, status, data);
    }
    pubnub_mutex_unlock(listener->mutw);
}

void pbcc_event_listener_emit_message(
    pbcc_event_listener_t*         listener,
    const char*                    subscribable,
    const struct pubnub_v2_message message)
{
    if (NULL == listener) { return; }

    pubnub_mutex_lock(listener->mutw);
    // Notify global message listeners (if any has been registered).
    if (NULL != listener->global_events) {
        pbcc_event_listener_emit_message_(listener,
                                          listener->global_events,
                                          message);
    }

    // Notify subscribable object message listeners (if any has been registered).
    const pbcc_object_listener_t* object_listener = (pbcc_object_listener_t*)
        pbhash_set_element(listener->listeners, subscribable);
    if (NULL != object_listener) {
        pbcc_event_listener_emit_message_(listener,
                                          object_listener->listeners,
                                          message);
    }
    pubnub_mutex_unlock(listener->mutw);
}

void pbcc_event_listener_free(pbcc_event_listener_t** event_listener)
{
    if (NULL == event_listener || NULL == *event_listener) { return; }

    pubnub_mutex_lock((*event_listener)->mutw);
    pbarray_free(&(*event_listener)->global_status);
    pbarray_free(&(*event_listener)->global_events);
    pbhash_set_free(&(*event_listener)->listeners);
    pubnub_mutex_unlock((*event_listener)->mutw);
    pubnub_mutex_destroy((*event_listener)->mutw);
    free(*event_listener);
    *event_listener = NULL;
}

pbcc_object_listener_t* pbcc_object_listener_alloc_(char* name)
{
    PBCC_ALLOCATE_TYPE(updates, pbcc_object_listener_t, true, NULL);
    updates->name = name;
    updates->listeners = NULL;
    pbcc_initialize_array_(
        &updates->listeners,
        (pbarray_element_free)pbcc_listener_free_);

    return updates;
}

enum pubnub_res pbcc_add_object_listener_(
    const pbcc_event_listener_t* event_listener,
    char*                        name,
    pbcc_listener_t*             listener)
{
    // Try to retrieve previously created object-scoped listeners.
    pbcc_object_listener_t* object_listener = (pbcc_object_listener_t*)
        pbhash_set_element(event_listener->listeners, name);
    if (NULL == object_listener) {
        object_listener = pbcc_object_listener_alloc_(name);
        if (NULL == object_listener) { return PNR_OUT_OF_MEMORY; }

        const pbhash_set_res rslt = pbhash_set_add(
            event_listener->listeners,
            name,
            object_listener);
        if (PBHSR_OUT_OF_MEMORY == rslt) {
            pbcc_object_listener_free_(object_listener);
            return PNR_OUT_OF_MEMORY;
        }
    }

    return pbcc_add_listener_(object_listener->listeners, listener);
}

enum pubnub_res pbcc_remove_object_listener_(
    const pbcc_event_listener_t* event_listener,
    char*                        name,
    const pbcc_listener_t*       listener)
{
    // Try to retrieve previously created object-scoped listeners.
    const pbcc_object_listener_t* object_listener = pbhash_set_element(
        event_listener->listeners,
        name);
    if (NULL == object_listener) { return PNR_OK; }

    const enum pubnub_res rslt = pbcc_remove_listener_(
        object_listener->listeners,
        listener);

    if (PNR_OK == rslt && 0 == pbarray_count(object_listener->listeners))
        pbhash_set_remove(event_listener->listeners, (void**)&name, NULL);

    return rslt;
}

void pbcc_object_listener_free_(pbcc_object_listener_t* listener)
{
    if (NULL == listener) { return; }

    if (NULL != listener->listeners) { pbarray_free(&listener->listeners); }
    if (NULL != listener->name) { free(listener->name); }
    free(listener);
}

pbcc_listener_t* pbcc_listener_alloc_(
    const void*                               subscription,
    const pubnub_subscribe_listener_type      type,
    const pubnub_subscribe_message_callback_t callback)
{
    PBCC_ALLOCATE_TYPE(listener, pbcc_listener_t, true, NULL);
    listener->counter             = pbref_counter_alloc();
    listener->type                = type;
    listener->subscription_object = subscription;
    listener->callback            = callback;

    return listener;
}

enum pubnub_res pbcc_add_listener_(
    pbarray_t*       listeners,
    pbcc_listener_t* listener)
{
    const pbarray_res rslt = pbarray_add(listeners, listener);
    if (PBAR_OK == rslt) { pbref_counter_increment(listener->counter); }

    return PBAR_OK == rslt ? PNR_OK : PNR_OUT_OF_MEMORY;
}

enum pubnub_res pbcc_remove_listener_(
    pbarray_t*             listeners,
    const pbcc_listener_t* listener)
{
    if (NULL == listeners) { return PNR_OK; }
    if (NULL == listener) { return PNR_INVALID_PARAMETERS; }

    for (size_t i = 0; i < pbarray_count(listeners);) {
        pbcc_listener_t* _listener = (pbcc_listener_t*)
            pbarray_element_at(listeners, i);

        if (_listener->type == listener->type &&
            _listener->subscription_object == listener->subscription_object &&
            _listener->callback == listener->callback) {
            pbref_counter_decrement(_listener->counter);
            pbarray_remove(listeners, (void**)&_listener, true);
        }
        else { i++; }
    }

    return PNR_OK;
}

void pbcc_listener_free_(pbcc_listener_t* listener)
{
    if (NULL == listener) { return; }
    if (0 == pbref_counter_free(listener->counter)) { free(listener); }
}

void pbcc_event_listener_emit_message_(
    const pbcc_event_listener_t*   listener,
    pbarray_t*                     listeners,
    const struct pubnub_v2_message message)
{
    const pubnub_subscribe_listener_type type =
        pbcc_message_type_to_listener_type_(message.message_type);
    const size_t count = pbarray_count(listeners);

    for (size_t i = 0; i < count; ++i) {
        pbcc_listener_t* _listener = (pbcc_listener_t*)
            pbarray_element_at(listeners, i);
        if (NULL == _listener->subscription_object || _listener->type == type)
            _listener->callback(listener->pb, message);
    }
}

pubnub_subscribe_listener_type pbcc_message_type_to_listener_type_(
    enum pubnub_message_type type)
{
    switch (type) {
    case pbsbPublished:
        return PBSL_LISTENER_ON_MESSAGE;
    case pbsbSignal:
        return PBSL_LISTENER_ON_SIGNAL;
    case pbsbAction:
        return PBSL_LISTENER_ON_MESSAGE_ACTION;
    case pbsbObjects:
        return PBSL_LISTENER_ON_OBJECTS;
    case pbsbFiles:
        return PBSL_LISTENER_ON_FILES;
    }

    return PBSL_LISTENER_ON_MESSAGE;
}

pbarray_t* pbcc_initialize_array_(
    pbarray_t**                array,
    const pbarray_element_free free_fn)
{
    if (NULL != *array) { return *array; }

    return *array = pbarray_alloc(5,
                                  PBARRAY_RESIZE_BALANCED,
                                  PBARRAY_GENERIC_CONTENT_TYPE,
                                  free_fn);
}