/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbcc_subscribe_event_engine.h"

#include <string.h>
#include <stdlib.h>

#include "core/pubnub_helper.h"
#include "core/pubnub_subscribe_event_engine_internal.h"
#include "core/pbcc_subscribe_event_engine_states.h"
#include "core/pbcc_subscribe_event_engine_events.h"
#include "core/pbcc_subscribe_event_engine_types.h"
#include "core/pbcc_subscribe_event_listener.h"
#include "core/pubnub_server_limits.h"
#include "core/pbcc_event_engine.h"
#include "core/pbcc_memory_utils.h"
#include "core/pubnub_mutex.h"
#include "core/pubnub_log.h"
#include "pubnub_callback.h"
#include "pubnub_internal.h"
#include "lib/pbhash_set.h"
#include "lib/pbstrdup.h"


// ----------------------------------------------
//                   Constants
// ----------------------------------------------

/** How many subscribable objects `pbcc_subscribe_ee_t` can hold by default. */
#define SUBSCRIBABLE_LENGTH 10


// ----------------------------------------------
//              Function prototypes
// ----------------------------------------------

/**
 * @brief Transit to `handshaking` / `receiving` state.
 *
 * Depending on from the flow, Subscribe Event Engine will transit to the
 * `receiving` state directly or through the `handshaking` state (if required).
 * Updated list of subscribables will be used to trigger proper event for
 * subscription loop.
 *
 * @param ee         Pointer to the Subscribe Event Engine, which should transit
 *                   to `receiving` state.
 * @param cursor     Pointer to the subscription cursor to be used with
 *                   subscribe REST API. The SDK will try to catch up on missed
 *                   messages if a cursor with older PubNub high-precision
 *                   timetoken has been provided. Pass `NULL` to keep using
 *                   cursor received from the previous subscribe REST API
 *                   response.
 * @param update     Whether list of subscribable objects should be updated or
 *                   not (for unsubscribe, it updated beforehand).
 * @param sent_by_ee Whether event has been sent by Subscribe Event Engine or
 *                   not.
 * @return Result of subscribe enqueue transaction.
 */
static enum pubnub_res pbcc_subscribe_ee_subscribe_(
    pbcc_subscribe_ee_t*             ee,
    const pubnub_subscribe_cursor_t* cursor,
    bool                             update,
    bool                             sent_by_ee);

/**
 * @brief Transit to `handshaking` / `receiving` or `unsubscribed` state.
 *
 * Depending on from whether there are any channels left for the next
 * subscription loop can be created, `SUBSCRIBE_EE_EVENT_UNSUBSCRIBE_ALL` event
 * instead of `EVENT_SUBSCRIPTION_CHANGE`.
 *
 * @param ee            Pointer to the Subscribe Event Engine, which should
 *                      transit to `handshaking` / `receiving` or `unsubscribed`
 *                      state.
 * @param subscribables Pointer to the hash with subscribables from which user
 *                      requested to unsubscribed.
 * @return Result of unsubscribe enqueue transaction.
 */
static enum pubnub_res pbcc_subscribe_ee_unsubscribe_(
    pbcc_subscribe_ee_t* ee,
    pbhash_set_t*        subscribables);

/**
 * @brief Perform postponed `unsubscribe` / `leave` operation.
 *
 * Subscribe Event Engine may save channels and groups from which it should
 * unsubscribe for later time because there is ongoing request. Saved
 * information will be used in this function to complete `leave` operation.
 *
 * @param ee Pointer to the Subscribe Event Engine, which may have information for `leave` request.
 * @return `true` in case if `leave` request sending has been scheduled.
 */
static bool pbcc_subscribe_ee_postponed_unsubscribe_(pbcc_subscribe_ee_t* ee);

/**
 * @brief Actualize list of subscribable objects.
 *
 * Update list of subscribable objects using `active` subscriptions and
 * subscription sets.
 *
 * @note Because of subscription / subscription set unsubscribe process, it is
 *       impossible to implement subscribable update in
 *       `_subscribe` / `_unsubscribe` methods. Subscribable addition, won't be
 *       a problem, but the removal process won't be able to figure out whether
 *       subscribable has been added by standalone subscribe object alone or
 *       another subscription object from subscription set tried to do the same.
 *       As a result, the object may be removed from the subscription loop.
 *
 * @param ee Pointer to the Subscribe Event Engine for which list of
 *           subscribable objects should be updated.
 * @return Result of subscribable list update.
 *
 * @see pubnub_subscription_t
 * @see pubnub_subscription_set_t
 */
static enum pubnub_res pbcc_subscribe_ee_update_subscribables_(
    const pbcc_subscribe_ee_t* ee);

/**
 * @brief Update Subscribe Event Engine list of subscribable objects.
 *
 * @param ee            Pointer to the Subscribe Event Engine for which
 *                      `subscribables` should be added.
 * @param subscribables Pointer to the hash set with subscribable objects which
 *                      is used for channels and channel groups list aggregation
 *                      for subscription loop.
 * @return Result of subscribables addition operation.
 */
static enum pubnub_res pbcc_subscribe_ee_add_subscribables_(
    const pbcc_subscribe_ee_t* ee,
    pbhash_set_t*              subscribables);

/**
 * @brief Get list of channel and group identifiers from provided subscribables.
 *
 * @param subscribables        Pointer to the hash set with unique subscribable
 *                             objects for which list of comma-separated
 *                             identifiers should be generated.
 * @param [out] channels       The parameter will hold a pointer to the
 *                             byte string with comma-separated / empty channel
 *                             identifiers for subscribe loop or `NULL` in case
 *                             of insufficient memory error.
 * @param [out] channel_groups The parameter will hold a pointer to the
 *                             byte string with comma-separated / empty channel
 *                             group identifiers for subscribe loop or `NULL` in
 *                             case of insufficient memory error.
 * @param include_presence     Whether channels with `-pnpres` should be
 *                             included into `channels` and `channel_groups`.
 * @return Result of subscribables computation operation.
 */
static enum pubnub_res pbcc_subscribe_ee_subscribables_(
    pbhash_set_t* subscribables,
    char**        channels,
    char**        channel_groups,
    bool          include_presence);

/**
 * @brief Handle real-time updates from subscription loop.
 *
 * @param pb        Pointer to the PubNub context which received updates in
 *                  subscription loop.
 * @param trans     Type of transaction for which callback has been called.
 * @param result    Result of `trans` processing
 * @param user_data Pointer to the Subscribe Event Engine object.
 */
static void pbcc_subscribe_callback_(
    pubnub_t*         pb,
    enum pubnub_trans trans,
    enum pubnub_res   result,
    const void*       user_data);

/**
 * @brief Create string from the `array` elements.
 *
 * @param array     Pointer to the array with string elements which should be
 *                  joined by the separator.
 * @param separator Pointer to the string which should be used to join elements.
 * @return Pointer to the string from the `array` elements or `NULL` in case of
 *         insufficient memory error. The returned pointer must be passed to the
 *         `free` to avoid a memory leak.
 */
static char* pbcc_subscribe_ee_joined_array_elements_(
    pbarray_t*  array,
    const char* separator);


// ----------------------------------------------
//                   Functions
// ----------------------------------------------

pbcc_subscribe_ee_t* pbcc_subscribe_ee_alloc(pubnub_t* pb)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    PBCC_ALLOCATE_TYPE(ee, pbcc_subscribe_ee_t, true, NULL);
    ee->subscription_sets = pbarray_alloc(
        SUBSCRIBABLE_LENGTH,
        PBARRAY_RESIZE_BALANCED,
        PBARRAY_GENERIC_CONTENT_TYPE,
        (pbarray_element_free)pubnub_subscription_set_free_);
    ee->subscriptions = pbarray_alloc(
        SUBSCRIBABLE_LENGTH,
        PBARRAY_RESIZE_BALANCED,
        PBARRAY_GENERIC_CONTENT_TYPE,
        (pbarray_element_free)pubnub_subscription_free_);
    ee->leave_channels = pbarray_alloc(
        SUBSCRIBABLE_LENGTH,
        PBARRAY_RESIZE_BALANCED,
        PBARRAY_CHAR_CONTENT_TYPE,
        (pbarray_element_free)free);
    ee->leave_channel_groups = pbarray_alloc(
        SUBSCRIBABLE_LENGTH,
        PBARRAY_RESIZE_BALANCED,
        PBARRAY_CHAR_CONTENT_TYPE,
        (pbarray_element_free)free);
    ee->current_transaction = PBTT_NONE;
    ee->cancel_invocation   = NULL;
    pubnub_mutex_init(ee->mutw);

    if (NULL == ee->subscriptions) {
        PUBNUB_LOG_ERROR("pbcc_subscribe_ee_alloc: failed to allocate memory "
            "for subscriptions list\n");
        pbcc_subscribe_ee_free(&ee);
        return NULL;
    }
    if (NULL == ee->subscription_sets) {
        PUBNUB_LOG_ERROR("pbcc_subscribe_ee_alloc: failed to allocate memory "
            "for subscription sets list\n");
        pbcc_subscribe_ee_free(&ee);
        return NULL;
    }
    if (NULL == ee->leave_channels) {
        PUBNUB_LOG_ERROR("pbcc_subscribe_ee_alloc: failed to allocate memory "
            "for unsubscribed channels\n");
        pbcc_subscribe_ee_free(&ee);
        return NULL;
    }
    if (NULL == ee->leave_channel_groups) {
        PUBNUB_LOG_ERROR("pbcc_subscribe_ee_alloc: failed to allocate memory "
            "for unsubscribed channel groups\n");
        pbcc_subscribe_ee_free(&ee);
        return NULL;
    }

    /** Prepare storage for computed list of subscribable objects. */
    ee->subscribables = pbhash_set_alloc(
        SUBSCRIBABLE_LENGTH,
        PBHASH_SET_CHAR_CONTENT_TYPE,
        (pbhash_set_element_free)pubnub_subscribable_free_);
    if (NULL == ee->subscribables) {
        PUBNUB_LOG_ERROR("pbcc_subscribe_ee_alloc: failed to allocate memory "
            "for subscribable objects\n");
        pbcc_subscribe_ee_free(&ee);
        return NULL;
    }
    ee->filter_expr = NULL;
    ee->heartbeat   = PUBNUB_DEFAULT_PRESENCE_HEARTBEAT_VALUE;
    ee->status      = PNSS_SUBSCRIPTION_STATUS_DISCONNECTED;
    ee->pb          = pb;

    /** Setup event engine */
    ee->ee = pbcc_ee_alloc(pbcc_unsubscribed_state_alloc());
    if (NULL == ee->ee) {
        PUBNUB_LOG_ERROR("pbcc_subscribe_ee_alloc: failed to allocate memory "
            "for event engine with initial state\n");
        pbcc_subscribe_ee_free(&ee);
        return NULL;
    }

    /** Setup event listener */
    ee->event_listener = pbcc_event_listener_alloc(pb);
    if (NULL == ee->ee) {
        PUBNUB_LOG_ERROR("pbcc_subscribe_ee_alloc: failed to allocate memory "
            "for event listener\n");
        pbcc_subscribe_ee_free(&ee);
        return NULL;
    }

    pubnub_register_callback(pb,
                             (pubnub_callback_t)pbcc_subscribe_callback_,
                             ee);

    return ee;
}

void pbcc_subscribe_ee_free(pbcc_subscribe_ee_t** ee)
{
    if (NULL == ee || NULL == *ee) { return; }

    pubnub_mutex_lock((*ee)->mutw);
    if (NULL != (*ee)->subscription_sets)
        pbarray_free(&(*ee)->subscription_sets);
    if (NULL != (*ee)->subscriptions) { pbarray_free(&(*ee)->subscriptions); }
    if (NULL != (*ee)->subscribables)
        pbhash_set_free(&(*ee)->subscribables);
    if (NULL != (*ee)->leave_channel_groups)
        pbarray_free(&(*ee)->leave_channel_groups);
    if (NULL != (*ee)->leave_channels) { pbarray_free(&(*ee)->leave_channels); }
    if (NULL != (*ee)->filter_expr) { free((*ee)->filter_expr); }
    if (NULL != (*ee)->ee) { pbcc_ee_free(&(*ee)->ee); }
    if (NULL != (*ee)->event_listener)
        pbcc_event_listener_free(&(*ee)->event_listener);
    pubnub_mutex_unlock((*ee)->mutw);
    pubnub_mutex_destroy((*ee)->mutw);
    free(*ee);
    *ee = NULL;
}

pbcc_event_listener_t* pbcc_subscribe_ee_event_listener(
    const pbcc_subscribe_ee_t* ee)
{
    return ee->event_listener;
}

pbcc_ee_data_t* pbcc_subscribe_ee_current_state_context(
    const pbcc_subscribe_ee_t* ee)
{
    pbcc_ee_state_t*      current_state = pbcc_ee_current_state(ee->ee);
    const pbcc_ee_data_t* data          = NULL;

    if (NULL != current_state) { data = pbcc_ee_state_data(current_state); }
    pbcc_ee_state_free(&current_state);

    return (pbcc_ee_data_t*)data;
}

void pbcc_subscribe_ee_set_filter_expression(
    pbcc_subscribe_ee_t* ee,
    const char*          filter_expr)
{
    PUBNUB_ASSERT_OPT(NULL != ee);

    pubnub_mutex_lock(ee->mutw);
    if (NULL != ee->filter_expr) { free(ee->filter_expr); }
    if (NULL == filter_expr) { ee->filter_expr = NULL; }
    else { ee->filter_expr = pbstrdup(filter_expr); }
    pubnub_mutex_unlock(ee->mutw);
}

void pbcc_subscribe_ee_set_heartbeat(
    pbcc_subscribe_ee_t* ee,
    const unsigned       heartbeat)
{
    PUBNUB_ASSERT_OPT(NULL != ee);

    pubnub_mutex_lock(ee->mutw);
    ee->heartbeat = PUBNUB_MINIMUM_PRESENCE_HEARTBEAT_VALUE > heartbeat
                        ? PUBNUB_MINIMUM_PRESENCE_HEARTBEAT_VALUE
                        : heartbeat;
    pubnub_mutex_unlock(ee->mutw);
}

enum pubnub_res pbcc_subscribe_ee_subscribe_with_subscription(
    pbcc_subscribe_ee_t*             ee,
    pubnub_subscription_t*           sub,
    const pubnub_subscribe_cursor_t* cursor)
{
    PUBNUB_ASSERT_OPT(NULL != ee);

    pubnub_mutex_lock(ee->mutw);
    if (PBAR_OK != pbarray_add(ee->subscriptions, sub)) {
        PUBNUB_LOG_ERROR("pbcc_subscribe_ee_subscribe_with_subscription: failed"
            " to allocate memory to store subscription\n");
        pubnub_mutex_unlock(ee->mutw);
        return PNR_OUT_OF_MEMORY;
    }

    const enum pubnub_res rslt = pbcc_subscribe_ee_subscribe_(
        ee,
        cursor,
        true,
        false);
    pubnub_mutex_unlock(ee->mutw);

    return rslt;
}

enum pubnub_res pbcc_subscribe_ee_unsubscribe_with_subscription(
    pbcc_subscribe_ee_t*         ee,
    const pubnub_subscription_t* sub)
{
    PUBNUB_ASSERT_OPT(NULL != ee);

    pubnub_mutex_lock(ee->mutw);
    if (!pbarray_contains(ee->subscriptions, sub)) {
        pubnub_mutex_unlock(ee->mutw);
        return PNR_SUB_NOT_FOUND;
    }

    pbhash_set_t* subs = pubnub_subscription_subscribables_(sub, NULL);

    if (NULL == subs) {
        PUBNUB_LOG_ERROR("pbcc_subscribe_ee_unsubscribe_with_subscription: "
            "failed to allocate memory for subscribables\n");
        pubnub_mutex_unlock(ee->mutw);
        return PNR_OUT_OF_MEMORY;
    }

    pbarray_remove(ee->subscriptions, (void**)&sub, true);
    const enum pubnub_res rslt = pbcc_subscribe_ee_unsubscribe_(ee, subs);

    /**
     * Subscribables list not needed anymore because channels / groups list
     * already composed for presence leave REST API in
     * `pbcc_subscribe_ee_unsubscribe_`.
     */
    pbhash_set_free_with_destructor(
        &subs,
        (pbhash_set_element_free)pubnub_subscribable_free_);
    pubnub_mutex_unlock(ee->mutw);

    return rslt;
}

enum pubnub_res pbcc_subscribe_ee_subscribe_with_subscription_set(
    pbcc_subscribe_ee_t*             ee,
    pubnub_subscription_set_t*       set,
    const pubnub_subscribe_cursor_t* cursor)
{
    PUBNUB_ASSERT_OPT(NULL != ee);

    pubnub_mutex_lock(ee->mutw);
    if (PBAR_OK != pbarray_add(ee->subscription_sets, set)) {
        PUBNUB_LOG_ERROR(
            "pbcc_subscribe_ee_subscribe_with_subscription_set: failed to "
            "allocate memory to store subscription set\n");
        pubnub_mutex_unlock(ee->mutw);
        return PNR_OUT_OF_MEMORY;
    }

    const enum pubnub_res rslt = pbcc_subscribe_ee_subscribe_(
        ee,
        cursor,
        true,
        false);
    pubnub_mutex_unlock(ee->mutw);

    return rslt;
}

enum pubnub_res pbcc_subscribe_ee_unsubscribe_with_subscription_set(
    pbcc_subscribe_ee_t*       ee,
    pubnub_subscription_set_t* set)
{
    PUBNUB_ASSERT_OPT(NULL != ee);

    pubnub_mutex_lock(ee->mutw);
    if (!pbarray_contains(ee->subscription_sets, set)) {
        pubnub_mutex_unlock(ee->mutw);
        return PNR_SUB_NOT_FOUND;
    }

    pbhash_set_t* subs = pubnub_subscription_set_subscribables_(set);

    if (NULL == subs) {
        PUBNUB_LOG_ERROR("pbcc_subscribe_ee_unsubscribe_with_subscription_set: "
            "failed to allocate memory for subscribables\n");
        pubnub_mutex_unlock(ee->mutw);
        return PNR_OUT_OF_MEMORY;
    }

    pbarray_remove(ee->subscription_sets, (void**)&set, true);
    const enum pubnub_res rslt = pbcc_subscribe_ee_unsubscribe_(ee, subs);

    /**
     * Subscribables list not needed anymore because channels / groups list
     * already composed for presence leave REST API in
     * `pbcc_subscribe_ee_unsubscribe_`.
     */
    pbhash_set_free_with_destructor(
        &subs,
        (pbhash_set_element_free)pubnub_subscribable_free_);
    pubnub_mutex_unlock(ee->mutw);

    return rslt;
}

enum pubnub_res pbcc_subscribe_ee_change_subscription_with_subscription_set(
    pbcc_subscribe_ee_t*             ee,
    const pubnub_subscription_set_t* set,
    const pubnub_subscription_t*     sub,
    const bool                       added)
{
    PUBNUB_ASSERT_OPT(NULL != ee);

    pubnub_mutex_lock(ee->mutw);
    enum pubnub_res rslt;

    if (added) { rslt = pbcc_subscribe_ee_subscribe_(ee, NULL, true, false); }
    else {
        const pubnub_subscription_options_t options =
            *(pubnub_subscription_options_t*)set;
        pbhash_set_t* subs = pubnub_subscription_subscribables_(sub, &options);

        if (NULL == subs) {
            PUBNUB_LOG_ERROR(
                "pbcc_subscribe_ee_change_subscription_with_subscription_set: "
                "failed to allocate memory for subscribables\n");
            pubnub_mutex_unlock(ee->mutw);
            return PNR_OUT_OF_MEMORY;
        }

        rslt = pbcc_subscribe_ee_unsubscribe_(ee, subs);
        /**
         * Subscribables list not needed anymore because channels / groups list
         * already composed for presence leave REST API in
         * `pbcc_subscribe_ee_unsubscribe_`.
         */
        pbhash_set_free_with_destructor(
            &subs,
            (pbhash_set_element_free)pubnub_subscribable_free_);
    }
    pubnub_mutex_unlock(ee->mutw);

    return rslt;
}

enum pubnub_res pbcc_subscribe_ee_disconnect(pbcc_subscribe_ee_t* ee)
{
    PUBNUB_ASSERT_OPT(NULL != ee);

    pubnub_mutex_lock(ee->mutw);
    pbcc_ee_event_t* event = pbcc_disconnect_event_alloc(ee);
    if (NULL == event) {
        PUBNUB_LOG_ERROR("pbcc_subscribe_ee_disconnect: failed to allocate "
            "memory for event\n");
        pubnub_mutex_unlock(ee->mutw);
        return PNR_OUT_OF_MEMORY;
    }
    pubnub_mutex_unlock(ee->mutw);

    return pbcc_ee_handle_event(ee->ee, event);
}

enum pubnub_res pbcc_subscribe_ee_reconnect(
    pbcc_subscribe_ee_t*            ee,
    const pubnub_subscribe_cursor_t cursor)
{
    PUBNUB_ASSERT_OPT(NULL != ee);

    pubnub_mutex_lock(ee->mutw);
    pbcc_ee_event_t* event = pbcc_reconnect_event_alloc(ee, cursor);
    if (NULL == event) {
        PUBNUB_LOG_ERROR("pbcc_subscribe_ee_reconnect: failed to allocate "
            "memory for event\n");
        pubnub_mutex_unlock(ee->mutw);
        return PNR_OUT_OF_MEMORY;
    }
    pubnub_mutex_unlock(ee->mutw);

    return pbcc_ee_handle_event(ee->ee, event);
}

enum pubnub_res pbcc_subscribe_ee_unsubscribe_all(pbcc_subscribe_ee_t* ee)
{
    PUBNUB_ASSERT_OPT(NULL != ee);

    pubnub_mutex_lock(ee->mutw);
    char *          ch, *cg;
    enum pubnub_res rslt = pbcc_subscribe_ee_subscribables_(ee->subscribables,
        &ch,
        &cg,
        false);

    if (PNR_OK == rslt) {
        pbcc_ee_event_t* event =
            pbcc_unsubscribe_all_event_alloc(ee, &ch, &cg);
        if (NULL == event) {
            PUBNUB_LOG_ERROR("pbcc_subscribe_ee_reconnect: failed to allocate "
                "memory for event\n");
            rslt = PNR_OUT_OF_MEMORY;

            if (NULL != ch) { free(ch); }
            if (NULL != cg) { free(cg); }
        }
        else {
            pbarray_remove_all(ee->subscription_sets);
            pbarray_remove_all(ee->subscriptions);

            /**
             * Update user presence information for channels which user actually
             * left.
             */
            if ((NULL != ch && 0 != strlen(ch)) ||
                (NULL != cg && 0 != strlen(cg))) {
                pubnub_mutex_lock(ee->pb->monitor);
                if (!pbnc_can_start_transaction(ee->pb)) {
                    pubnub_mutex_unlock(ee->pb->monitor);
                    /** Using array to handle consequencive call to unsubscribe. */
                    pubnub_mutex_lock(ee->mutw);
                    if (NULL != ch && strlen(ch) > 0)
                        pbarray_add(ee->leave_channels, ch);
                    if (NULL != cg && strlen(cg) > 0)
                        pbarray_add(ee->leave_channel_groups, cg);
                    pubnub_mutex_unlock(ee->mutw);
                }
                else {
                    pubnub_mutex_unlock(ee->pb->monitor);

                    pubnub_mutex_lock(ee->mutw);
                    ee->current_transaction = PBTT_LEAVE;
                    pubnub_mutex_unlock(ee->mutw);

                    pubnub_leave(ee->pb,
                                 0 == strlen(ch) ? NULL : ch,
                                 0 == strlen(cg) ? NULL : cg);
                }
            }

            rslt = pbcc_ee_handle_event(ee->ee, event);
        }
    }
    pubnub_mutex_unlock(ee->mutw);

    /** Looks like there is nothing to unsubscribe from. */
    if (PNR_INVALID_PARAMETERS == rslt) { rslt = PNR_OK; }

    return rslt;
}

void pbcc_subscribe_ee_handle_subscribe_error(
    pbcc_subscribe_ee_t*  ee,
    const enum pubnub_res error)
{
    pbcc_ee_event_t* event        = NULL;
    pbcc_ee_state_t* state_object = pbcc_ee_current_state(ee->ee);

    if (SUBSCRIBE_EE_STATE_HANDSHAKING == pbcc_ee_state_type(state_object))
        event = pbcc_handshake_failure_event_alloc(ee, error);
    else if (SUBSCRIBE_EE_STATE_RECEIVING == pbcc_ee_state_type(state_object))
        event = pbcc_receive_failure_event_alloc(ee, error);

    if (NULL != event) { pbcc_ee_handle_event(ee->ee, event); }
    if (NULL != state_object) { pbcc_ee_state_free(&state_object); }
}

pubnub_subscription_t** pbcc_subscribe_ee_subscriptions(
    pbcc_subscribe_ee_t* ee,
    size_t*              count)
{
    PUBNUB_ASSERT_OPT(NULL != ee);

    pubnub_mutex_lock(ee->mutw);
    pubnub_subscription_t** subs = (pubnub_subscription_t**)
        pbarray_elements(ee->subscriptions, count);
    pubnub_mutex_unlock(ee->mutw);

    return subs;
}

pubnub_subscription_set_t** pbcc_subscribe_ee_subscription_sets(
    pbcc_subscribe_ee_t* ee,
    size_t*              count)
{
    PUBNUB_ASSERT_OPT(NULL != ee);

    pubnub_mutex_lock(ee->mutw);
    pubnub_subscription_set_t** subs = (pubnub_subscription_set_t**)
        pbarray_elements(ee->subscription_sets, count);
    pubnub_mutex_unlock(ee->mutw);

    return subs;
}

void pbcc_subscribe_callback_(
    pubnub_t*               pb,
    const enum pubnub_trans trans,
    const enum pubnub_res   result,
    const void*             user_data
    )
{
    pbcc_subscribe_ee_t* ee = (pbcc_subscribe_ee_t*)user_data;

    /**
     * Checking whether a leave request should be performed if a previous
     * (potentially subscribe) request has been cancelled or was another leave
     * request.
     */
    if (PNR_CANCELLED == result || PBTT_HEARTBEAT == trans ||
        PBTT_LEAVE == trans) {
        pubnub_mutex_lock(ee->mutw);
        if (NULL != ee->cancel_invocation) {
            pbcc_ee_handle_effect_completion(ee->ee, ee->cancel_invocation);
            ee->cancel_invocation = NULL;
        }
        if (PBTT_HEARTBEAT == trans || PBTT_LEAVE == trans) {
            ee->current_transaction = PBTT_NONE;
            /** Flush read buffer. */
            pubnub_get(ee->pb);
        }
        pubnub_mutex_unlock(ee->mutw);

        if (pbcc_subscribe_ee_postponed_unsubscribe_(ee)) { return; }

        /**
         * Leave or heartbeat request successfully processed. Trying schedule
         * any pending effect invocations.
         */
        if (0 != pbcc_ee_process_next_invocation(ee->ee)) { return; }
    }

    const bool                error        = PNR_OK != result;
    pbcc_ee_state_t*          state_object = pbcc_ee_current_state(ee->ee);
    pbcc_ee_event_t*          event        = NULL;
    pubnub_subscribe_cursor_t cursor;
    cursor.region = 0;

    if (!error) {
        /** Retrieve parsed timetoken stored for next subscription loop. */
        pubnub_mutex_lock(pb->monitor);
        size_t token_len = strlen(pb->core.timetoken);
        memcpy(cursor.timetoken, pb->core.timetoken, token_len);
        cursor.timetoken[token_len] = '\0';
        if (pb->core.region > 0) { cursor.region = pb->core.region; }
        pbpal_mutex_unlock(pb->monitor);
    }

    if (SUBSCRIBE_EE_STATE_HANDSHAKING == pbcc_ee_state_type(state_object)) {
        if (!error) { event = pbcc_handshake_success_event_alloc(ee, cursor); }
        else { event = pbcc_handshake_failure_event_alloc(ee, result); }
    }
    else if (SUBSCRIBE_EE_STATE_RECEIVING == pbcc_ee_state_type(state_object)) {
        if (!error) { event = pbcc_receive_success_event_alloc(ee, cursor); }
        else { event = pbcc_receive_failure_event_alloc(ee, result); }
    }

    if (NULL != event) { pbcc_ee_handle_event(ee->ee, event); }
    if (NULL != state_object) { pbcc_ee_state_free(&state_object); }
}

enum pubnub_res pbcc_subscribe_ee_subscribe_(
    pbcc_subscribe_ee_t*             ee,
    const pubnub_subscribe_cursor_t* cursor,
    const bool                       update,
    const bool                       sent_by_ee)
{
    pbcc_ee_event_t* event = NULL;
    char *           ch    = NULL, *cg = NULL;
    enum pubnub_res  rslt  = PNR_OK;

    if (update) { rslt = pbcc_subscribe_ee_update_subscribables_(ee); }
    if (PNR_OK == rslt) {
        rslt = pbcc_subscribe_ee_subscribables_(ee->subscribables,
                                                &ch,
                                                &cg,
                                                true);

        /**
         * Empty list allowed and will mean that event engine should transit to
         * the `Unsubscribed` state.
         */
        if (PNR_INVALID_PARAMETERS == rslt) {
            if (NULL != ch) { free(ch); }
            if (NULL != cg) { free(cg); }
            ch   = NULL;
            cg   = NULL;
            rslt = PNR_OK;
        }
    }

    if (PNR_OK == rslt) {
        const bool restore = NULL != cursor && '0' != cursor->timetoken[0];

        if (NULL == cursor || !restore)
            event = pbcc_subscription_changed_event_alloc(
                ee,
                &ch,
                &cg,
                sent_by_ee);
        else
            event = pbcc_subscription_restored_event_alloc(
                ee,
                &ch,
                &cg,
                *cursor,
                sent_by_ee);
        if (NULL == event) {
            PUBNUB_LOG_ERROR(
                "pbcc_subscribe_ee_subscribe: failed to allocate memory for "
                "event\n");
            if (NULL != ch) { free(ch); }
            if (NULL != cg) { free(cg); }
            rslt = PNR_OUT_OF_MEMORY;
        }
    }

    // TODO: We probably will need presence event engine implementation.
    /**
     * `pubnub_heartbeat` not used separately because `pubnub_subscribe_v2`
     * internally use automated heartbeat feature.
     */

    return PNR_OK == rslt ? pbcc_ee_handle_event(ee->ee, event) : rslt;
}

enum pubnub_res pbcc_subscribe_ee_unsubscribe_(
    pbcc_subscribe_ee_t* ee,
    pbhash_set_t*        subscribables)
{
    char *ch         = NULL, *cg = NULL;
    bool  send_leave = false;

    size_t                  count = 0;
    pubnub_subscribable_t** subs  = (pubnub_subscribable_t**)
        pbhash_set_elements(subscribables, &count);
    if (NULL == subs) { return PNR_OUT_OF_MEMORY; }

    /**
     * After subscription or subscription set removal we need to update
     * subscribables list.
     */
    enum pubnub_res rslt = pbcc_subscribe_ee_update_subscribables_(ee);
    if (PNR_OK != rslt) {
        free(subs);
        return rslt;
    }

    /** Removing subscribable which is not part of subscription loop. */
    for (size_t i = 0; i < count; ++i) {
        pubnub_subscribable_t* sub = subs[i];

        if (pbhash_set_contains(ee->subscribables, sub->id->ptr)) {
            pbhash_set_remove(subscribables,
                              (void**)&sub->id->ptr,
                              (void**)&sub);
            pubnub_subscribable_free_(sub);
        }
    }
    free(subs);

    rslt = pbcc_subscribe_ee_subscribables_(subscribables, &ch, &cg, false);
    if (PNR_OK == rslt || PNR_INVALID_PARAMETERS == rslt) {
        send_leave = PNR_OK == rslt;
        rslt       = PNR_OK;
    }

    /**
     * Update user presence information for channels which user actually
     * left.
     */
    bool sending_leave = false;
    if (PNR_OK == rslt && send_leave) {
        pubnub_mutex_lock(ee->pb->monitor);
        if (!pbnc_can_start_transaction(ee->pb)) {
            pubnub_mutex_unlock(ee->pb->monitor);
            /** Using array to handle consequencive call to unsubscribe. */
            pubnub_mutex_lock(ee->mutw);
            if (NULL != ch && strlen(ch) > 0)
                pbarray_add(ee->leave_channels, ch);
            if (NULL != cg && strlen(cg) > 0)
                pbarray_add(ee->leave_channel_groups, cg);
            pubnub_mutex_unlock(ee->mutw);
        }
        else {
            pubnub_mutex_unlock(ee->pb->monitor);

            pubnub_mutex_lock(ee->mutw);
            ee->current_transaction = PBTT_LEAVE;
            pubnub_mutex_unlock(ee->mutw);
            sending_leave = true;

            pubnub_leave(ee->pb,
                         0 == strlen(ch) ? NULL : ch,
                         0 == strlen(cg) ? NULL : cg);
        }
    }

    if (PNR_OK != rslt || !send_leave || sending_leave) {
        if (NULL != ch) { free(ch); }
        if (NULL != cg) { free(cg); }
    }

    /** Update subscription loop. */
    if (PNR_OK == rslt)
        rslt = pbcc_subscribe_ee_subscribe_(ee, NULL, false, true);

    return rslt;
}

bool pbcc_subscribe_ee_postponed_unsubscribe_(pbcc_subscribe_ee_t* ee)
{
    pubnub_mutex_lock(ee->mutw);
    if (0 == pbarray_count(ee->leave_channels) &&
        0 == pbarray_count(ee->leave_channel_groups)) {
        pubnub_mutex_unlock(ee->mutw);
        return false;
    }

    /**
     * Checking whether there is another ongoing request and we need to wait
     * till its completion to start another one.
     */
    if (PBTT_NONE != ee->current_transaction) {
        pubnub_mutex_unlock(ee->mutw);
        return true;
    }

    char* ch =
        pbcc_subscribe_ee_joined_array_elements_(ee->leave_channels, ",");
    char* cg =
        pbcc_subscribe_ee_joined_array_elements_(ee->leave_channel_groups, ",");

    /** Cleaning up postponed channels and groups. */
    pbarray_remove_all(ee->leave_channels);
    pbarray_remove_all(ee->leave_channel_groups);

    if (NULL == ch && NULL == cg) {
        pubnub_mutex_unlock(ee->mutw);
        return false;
    }

    ee->current_transaction = PBTT_LEAVE;
    pubnub_mutex_unlock(ee->mutw);

    pubnub_leave(ee->pb,
                 NULL == ch || 0 == strlen(ch) ? NULL : ch,
                 NULL == cg || 0 == strlen(cg) ? NULL : cg);
    if (NULL != ch) { free(ch); }
    if (NULL != cg) { free(cg); }

    return true;
}

enum pubnub_res pbcc_subscribe_ee_update_subscribables_(
    const pbcc_subscribe_ee_t* ee)
{
    PUBNUB_ASSERT_OPT(NULL != ee);
    pbarray_t*      sets = ee->subscription_sets;
    pbarray_t*      subs = ee->subscriptions;
    enum pubnub_res rslt = PNR_OK;

    /** Clean up previous set to start with fresh list. */
    pbhash_set_remove_all(ee->subscribables);

    for (int i = 0; i < pbarray_count(subs); ++i) {
        const pubnub_subscription_t* sub = pbarray_element_at(subs, i);
        pbhash_set_t* subc = pubnub_subscription_subscribables_(sub, NULL);
        rslt = pbcc_subscribe_ee_add_subscribables_(ee, subc);
        pbhash_set_free(&subc);
        if (PNR_OK != rslt) { return rslt; }
    }

    for (int i = 0; i < pbarray_count(sets); ++i) {
        const pubnub_subscription_set_t* sub = pbarray_element_at(sets, i);
        pbhash_set_t* subc = pubnub_subscription_set_subscribables_(sub);
        rslt = pbcc_subscribe_ee_add_subscribables_(ee, subc);
        pbhash_set_free(&subc);
        if (PNR_OK != rslt) { return rslt; }
    }

    return rslt;
}

enum pubnub_res pbcc_subscribe_ee_add_subscribables_(
    const pbcc_subscribe_ee_t* ee,
    pbhash_set_t*              subscribables)
{
    PUBNUB_ASSERT_OPT(NULL != ee);
    PUBNUB_ASSERT_OPT(NULL != subscribables);

    enum pubnub_res rslt = PNR_OK;
    pbhash_set_t*   dups = NULL;

    if (NULL == subscribables ||
        PBHSR_OUT_OF_MEMORY == pbhash_set_union(
            ee->subscribables,
            subscribables,
            &dups)) {
        if (NULL == subscribables) {
            PUBNUB_LOG_ERROR(
                "pbcc_subscribe_ee_add_subscribables: failed to allocate "
                "memory for subscription's subscribables\n");
        }
        else {
            PUBNUB_LOG_ERROR(
                "pbcc_subscribe_ee_add_subscribables: failed to allocate "
                "memory to store subscribables\n");
        }

        rslt = PNR_OUT_OF_MEMORY;
    }

    if (NULL == dups) { return rslt; }

    /** Make sure that duplicates will be freed. */
    size_t                  dups_count = 0;
    pubnub_subscribable_t** elms       = (pubnub_subscribable_t**)
        pbhash_set_elements(dups, &dups_count);
    for (size_t j = 0; j < dups_count; ++j) {
        pubnub_subscribable_t* elm = elms[j];

        if (PBHSR_VALUE_EXISTS == pbhash_set_match_element(
                ee->subscribables,
                elm)) {
            pubnub_subscribable_free_(elm);
        }
    }
    if (NULL != elms) { free(elms); }
    pbhash_set_free(&dups);

    return rslt;
}

enum pubnub_res pbcc_subscribe_ee_subscribables_(
    pbhash_set_t* subscribables,
    char**        channels,
    char**        channel_groups,
    const bool    include_presence)
{
    PUBNUB_ASSERT_OPT(NULL != subscribables);

    /** Expected length of comma-separated channel names. */
    size_t ch_list_length = 0, ch_count = 0, ch_offset = 0;
    /** Expected length of comma-separated channel group names. */
    size_t cg_list_length = 0, cg_count = 0, cg_offset = 0;

    size_t                        count;
    const pubnub_subscribable_t** subs = (const pubnub_subscribable_t**)
        pbhash_set_elements(subscribables, &count);

    for (int i = 0; i < count; ++i) {
        const pubnub_subscribable_t* sub = subs[i];
        /** Ignoring presence subscribables if they shouldn't be included. */
        if (!include_presence && pubnub_subscribable_is_presence_(sub))
            continue;

        if (!pubnub_subscribable_is_cg_(sub)) {
            ch_list_length += pubnub_subscribable_length_(sub);
            ch_count++;
        }
        else {
            cg_list_length += pubnub_subscribable_length_(sub);
            cg_count++;
        }
    }

    /** Extra byte for null-terminator. */
    const size_t ch_len = ch_list_length + ch_count + 1;
    const size_t cg_len = cg_list_length + cg_count + 1;
    *channels           = calloc(ch_len, sizeof(char));
    *channel_groups     = calloc(cg_len, sizeof(char));

    if (NULL == *channels || NULL == *channel_groups) {
        PUBNUB_LOG_ERROR(
            "pbcc_subscribe_ee_subscribables: failed to allocate memory to "
            "store subscription channels / channel groups\n");
        if (NULL != *channel_groups) {
            free(*channel_groups);
            *channel_groups = NULL;
        }
        if (NULL != *channels) {
            free(*channels);
            *channels = NULL;
        }
        if (NULL != subs) { free(subs); }
        return PNR_OUT_OF_MEMORY;
    }

    if (0 == cg_count) { *channel_groups[0] = '\0'; }
    if (0 == ch_count) { *channels[0] = '\0'; }

    for (int i = 0; i < count; ++i) {
        const pubnub_subscribable_t* sub = subs[i];
        /** Ignoring presence subscribables if they shouldn't be included. */
        if (!include_presence && pubnub_subscribable_is_presence_(sub))
            continue;

        if (!pubnub_subscribable_is_cg_(sub)) {
            ch_offset += snprintf(*channels + ch_offset,
                                  ch_len - ch_offset,
                                  "%s%s",
                                  ch_offset > 0 ? "," : "",
                                  sub->id->ptr);
        }
        else {
            cg_offset += snprintf(*channel_groups + cg_offset,
                                  cg_len - cg_offset,
                                  "%s%s",
                                  cg_offset > 0 ? "," : "",
                                  sub->id->ptr);
        }
    }
    if (NULL != subs) { free(subs); }

    return ch_list_length == 0 && cg_list_length == 0
               ? PNR_INVALID_PARAMETERS
               : PNR_OK;
}

char* pbcc_subscribe_ee_joined_array_elements_(
    pbarray_t*  array,
    const char* separator)
{
    size_t offset = 0;
    size_t count;
    char** elements = (char**)pbarray_elements(array, &count);
    if (NULL == elements) { return NULL; }

    /** Extra byte for null-terminator. */
    size_t len = (count > 0 ? (count - 1) * strlen(separator) : 0) + 1;
    for (size_t i = 0; i < count; ++i) { len += strlen(elements[i]); }

    char* joined_str = malloc(len * sizeof(char));
    if (NULL == joined_str || 0 == count) {
        if (NULL != joined_str) { joined_str[0] = '\0'; }
        free(elements);
        return joined_str;
    }

    for (int i = 0; i < count; ++i) {
        offset += snprintf(joined_str + offset,
                           len - offset,
                           "%s%s",
                           offset > 0 ? separator : "",
                           elements[i]);
    }

    return joined_str;
}