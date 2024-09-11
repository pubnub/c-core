#include "pubnub_subscribe_event_engine_internal.h"

#include <stdlib.h>

#include "pbcc_memory_utils.h"
#include "core/pubnub_entities_internal.h"
#include "core/pubnub_internal_common.h"
#include "core/pubnub_assert.h"
#include "core/pubnub_mutex.h"
#include "core/pubnub_log.h"
#include "lib/pbhash_set.h"


// ----------------------------------------------
//                   Constants
// ----------------------------------------------

// Pre-allocated subscription object pointers array length.
#define SUBSCRIPTIONS_LENGTH 10


// ----------------------------------------------
//             Function prototypes
// ----------------------------------------------

/**
 * @brief Create entity subscription.
 *
 * Subscription is high-level access to the entity which is used in the
 * subscription loop. Subscriptions make it possible to add real-time update
 * listeners for specific entity (channel, group, or App Context object). It is
 * possible to `subscribe` and `unsubscribe` using a specific subscription.
 *
 * @note Pointer to the subscriptions later can be received with
 *       `pubnub_subscriptions` function.
 *
 * @param entity     Pointer to the PubNub entity which should be used in
 *                   subscription.
 * @param options    Pointer to the subscription configuration options. Set to
 *                   `NULL` if options not required.
 * @param standalone Whether a subscription object is standalone or part of set.
 * @return Pointer to the ready to use PubNub subscription, or `NULL` will be
 *         returned in case of error (in most of the cases caused by
 *         insufficient memory for structure allocation). The returned pointer
 *         must be passed to the `pubnub_subscription_free` to avoid a memory
 *         leak.
 *
 * @see pubnub_subscriptions
 */
static pubnub_subscription_t* _pubnub_subscription_alloc(
    pubnub_entity_t*                     entity,
    const pubnub_subscription_options_t* options,
    bool                                 standalone);

/**
 * @brief Update number references to the `subscription`.
 *
 * \b Warning: Application execution will be terminated if `subscription` is
 * `NULL`.
 *
 * @param subscription Subscription object for which number references should be
 *                     updated.
 * @param increase     Whether reference count should be increased or decreased.
 */
static void _subscription_reference_count_update(
    pubnub_subscription_t* subscription,
    bool                   increase);

/**
 * @brief Create a subscription set.
 *
 * @param options  Subscription configuration options. Set to `NULL` if options
 *                 not required.
 * @param length   Desired length of a pre-allocated set for subscription
 *                 pointers.
 * @return Ready to use a subscription set object, or `NULL` will be returned in
 *         case of error (in most of the cases caused by insufficient memory for
 *         structure allocation). The returned pointer must be passed to the
 *         `pubnub_subscription_set_free` to avoid a memory leak.
 */
static pubnub_subscription_set_t* _subscription_set_alloc(
    const pubnub_subscription_options_t* options,
    int                                  length);

/**
* @brief Update the number of subscription set which refer to `subscription`.
 *
 * \b Warning: Application execution will be terminated if `set` is `NULL`.
 *
 * @param set      Subscription set object for which number references should be
 *                 updated.
 * @param increase Whether reference count should be increased or decreased.
 */
static void _subscription_set_reference_count_update(
    pubnub_subscription_set_t* set,
    bool                       increase);

/**
 * @brief Create subscribable object.
 *
 * @param location Subscribable identifier location in subscribe REST API URI.
 * @param id       Subscribable identifier.
 * @param presence Whether subscribable represent presence events stream or not.
 * @return Ready to use a subscribable object. The returned pointer
 *         must be passed to the `_pubnub_subscribable_free` to avoid a memory
 *         leak.
 */
static pubnub_subscribable_t* _pubnub_subscribable_alloc(
    enum pubnub_subscribable_location location,
    struct pubnub_char_mem_block*     id,
    bool                              presence);


// ----------------------------------------------
//                   Functions
// ----------------------------------------------

pubnub_subscription_options_t pubnub_subscription_options_defopts(void)
{
    return (pubnub_subscription_options_t){ false };
}

pubnub_subscription_t* pubnub_subscription_alloc(
    pubnub_entity_t*                     entity,
    const pubnub_subscription_options_t* options)
{
    return _pubnub_subscription_alloc(entity, options, true);
}

bool pubnub_subscription_free(pubnub_subscription_t* sub)
{
    if (NULL == sub) { return false; }

    _subscription_reference_count_update(sub, false);
    pubnub_mutex_lock(sub->mutw);
    if (sub->reference_count == 0) {
        pubnub_entity_free(sub->entity);
        pubnub_mutex_unlock(sub->mutw);
        pubnub_mutex_destroy(sub->mutw);
        free(sub);
        return true;
    }
    pubnub_mutex_unlock(sub->mutw);

    return false;
}

pubnub_subscription_t** pubnub_subscriptions(const pubnub_t* pb, size_t* count)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    size_t                  cnt           = 0;
    pubnub_subscription_t** subscriptions = pbcc_subscribe_ee_subscriptions(
        pb->core.subscribe_ee,
        &cnt);

    if (NULL != count) { *count = cnt; }
    if (NULL == subscriptions) {
        PUBNUB_LOG_ERROR("pubnub_subscriptions: failed to allocate memory for "
                         "subscriptions list\n");
    } else if (0 == cnt) {
        PUBNUB_LOG_INFO("pubnub_subscriptions: subscriptions list is empty");
    }

    return subscriptions;
}

pubnub_subscription_set_t*
pubnub_subscription_set_alloc_with_entities(
    pubnub_entity_t**                    entities,
    const int                            entities_count,
    const pubnub_subscription_options_t* options)
{
    PUBNUB_ASSERT_OPT(NULL != entities);

    const int length = entities_count - entities_count % SUBSCRIPTIONS_LENGTH +
                       SUBSCRIPTIONS_LENGTH;
    const pubnub_t*            pb  = NULL;
    pubnub_subscription_set_t* set = _subscription_set_alloc(options, length);
    if (NULL == set) { return NULL; }

    for (int i = 0; i < entities_count; ++i) {
        pubnub_entity_t*       entity = entities[i];
        pubnub_subscription_t* sub    = _pubnub_subscription_alloc(
            entity,
            NULL,
            false);

        if (NULL == pb) { pb = entity->pb; }
        if (NULL == sub ||
            PNR_OUT_OF_MEMORY == pubnub_subscription_set_add(set, sub)) {
            if (NULL != sub) { pubnub_subscription_free(sub); }
            pubnub_subscription_set_free(set);
            return NULL;
        }
    }

    if (pb) { set->ee = pb->core.subscribe_ee; }
    else {
        PUBNUB_LOG_ERROR(
            "pubnub_subscription_set_alloc_with_entities: invalid PubNub "
            "context\n");
        pubnub_subscription_set_free(set);
        return NULL;
    }

    return set;
}

pubnub_subscription_set_t*
pubnub_subscription_set_alloc_with_subscriptions(
    pubnub_subscription_t*               sub1,
    pubnub_subscription_t*               sub2,
    const pubnub_subscription_options_t* options)
{
    PUBNUB_ASSERT_OPT(NULL != sub1);
    PUBNUB_ASSERT_OPT(NULL != sub2);

    pubnub_subscription_set_t* set = _subscription_set_alloc(
        options,
        SUBSCRIPTIONS_LENGTH);
    if (NULL == set) { return NULL; }

    if (PNR_OUT_OF_MEMORY == pubnub_subscription_set_add(set, sub1) ||
        PNR_OUT_OF_MEMORY == pubnub_subscription_set_add(set, sub2)) {
        pubnub_subscription_set_free(set);
        return NULL;
    }

    set->ee = sub1->ee;

    return set;
}

enum pubnub_res pubnub_subscription_set_add(
    pubnub_subscription_set_t* set,
    pubnub_subscription_t*     sub)
{
    PUBNUB_ASSERT_OPT(NULL != set);
    PUBNUB_ASSERT_OPT(NULL != sub);

    pubnub_mutex_lock(set->mutw);
    const bool subscribed = set->subscribed;

    if (pbhash_set_contains(set->subscriptions, sub->entity->id.ptr)) {
        PUBNUB_LOG_INFO(
            "pubnub_subscription_set_add: subscription (sub=%p) for entity "
            "('%s') already added into set",
            sub,
            sub->entity->id.ptr);
        pubnub_mutex_unlock(set->mutw);
        return PNR_SUB_ALREADY_ADDED;

    }

    if (PBHSR_OUT_OF_MEMORY == pbhash_set_add(set->subscriptions,
                                              sub->entity->id.ptr,
                                              sub)) {
        pubnub_mutex_unlock(set->mutw);
        PUBNUB_LOG_ERROR(
            "pubnub_subscription_set_add: failed to allocate memory "
            "for subscription\n");
        return PNR_OUT_OF_MEMORY;
    }

    _subscription_reference_count_update(sub, true);
    pubnub_mutex_unlock(set->mutw);

    if (subscribed) {
        // Notify changes in active subscription set.
        pbcc_subscribe_ee_change_subscription_with_subscription_set(
            set->ee,
            set,
            sub,
            true);
    }

    return PNR_OK;
}

enum pubnub_res pubnub_subscription_set_remove(
    pubnub_subscription_set_t* set,
    pubnub_subscription_t*     sub)
{
    PUBNUB_ASSERT_OPT(NULL != set);
    PUBNUB_ASSERT_OPT(NULL != sub);

    pubnub_mutex_lock(set->mutw);
    const bool subscribed = set->subscribed;

    if (!pbhash_set_contains(set->subscriptions, sub->entity->id.ptr)) {
        PUBNUB_LOG_INFO(
            "pubnub_subscription_set_remove: subscription (sub=%p) for entity "
            "('%s') not found",
            sub,
            sub->entity->id.ptr);
        pubnub_mutex_unlock(set->mutw);
        return PNR_SUB_NOT_FOUND;
    }

    if (subscribed) {
        // Temporarily release lock to avoid deadlocks from subscription ee core
        // code access the list of subscribables.
        pubnub_mutex_unlock(set->mutw);

        // Notify changes in active subscription set.
        pbcc_subscribe_ee_change_subscription_with_subscription_set(
            set->ee,
            set,
            sub,
            false);
        pubnub_mutex_lock(set->mutw);
        pbhash_set_remove(set->subscriptions, sub->entity->id.ptr);
    }
    else { pbhash_set_remove(set->subscriptions, sub->entity->id.ptr); }
    pubnub_mutex_unlock(set->mutw);

    return PNR_OK;
}

enum pubnub_res pubnub_subscription_set_union(
    pubnub_subscription_set_t*       set,
    const pubnub_subscription_set_t* other_set)
{
    PUBNUB_ASSERT_OPT(NULL != set);
    PUBNUB_ASSERT_OPT(NULL != other_set);

    pubnub_mutex_lock(other_set->mutw);
    size_t                  count;
    pubnub_subscription_t** subs = (pubnub_subscription_t**)
        pbhash_set_elements(set->subscriptions, &count);

    for (int i = 0; i < count; ++i) {
        if (PNR_OUT_OF_MEMORY == pubnub_subscription_set_add(set, subs[i])) {
            pubnub_mutex_unlock(other_set->mutw);
            if (NULL != subs) { free(subs); }
            return PNR_OUT_OF_MEMORY;
        }
    }
    pubnub_mutex_unlock(other_set->mutw);
    if (NULL != subs) { free(subs); }

    return PNR_OK;
}

void pubnub_subscription_set_subtract(
    pubnub_subscription_set_t*       set,
    const pubnub_subscription_set_t* other_set)
{
    PUBNUB_ASSERT_OPT(NULL != set);
    PUBNUB_ASSERT_OPT(NULL != other_set);

    pubnub_mutex_lock(other_set->mutw);
    size_t                  count;
    pubnub_subscription_t** subs = (pubnub_subscription_t**)
        pbhash_set_elements(set->subscriptions, &count);

    for (int i = 0; i < count; ++i) {
        pubnub_subscription_set_remove(set, subs[i]);
    }
    pubnub_mutex_unlock(other_set->mutw);
    if (NULL != subs) { free(subs); }
}

pubnub_subscription_set_t** pubnub_subscription_sets(
    const pubnub_t* pb,
    size_t*         count)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    size_t                      cnt  = 0;
    pubnub_subscription_set_t** sets = pbcc_subscribe_ee_subscription_sets(
        pb->core.subscribe_ee,
        &cnt);

    if (NULL != count) { *count = cnt; }
    if (0 == cnt) {
        PUBNUB_LOG_INFO(
            "pubnub_subscription_sets: subscriptions list is empty");
    }
    else if (NULL == sets) {
        PUBNUB_LOG_ERROR("pubnub_subscription_sets: failed to allocate memory");
    }

    return sets;
}

pubnub_subscription_t** pubnub_subscription_set_subscriptions(
    const pubnub_subscription_set_t* set,
    size_t*                          count)
{
    PUBNUB_ASSERT_OPT(NULL != set);

    pubnub_mutex_lock(set->mutw);
    size_t                        cnt;
    const pubnub_subscription_t** subs = (const pubnub_subscription_t**)
        pbhash_set_elements(set->subscriptions, &cnt);

    if (NULL != count) { *count = cnt; }
    if (NULL == subs) {
        PUBNUB_LOG_ERROR("pubnub_subscription_set_subscriptions: failed to "
            "allocate memory for subscriptions list\n");
    } else if (0 == cnt) {
        PUBNUB_LOG_INFO(
            "pubnub_subscription_set_subscriptions: subscriptions list is "
            "empty");
    }
    pubnub_mutex_unlock(set->mutw);

    return subs;
}

bool pubnub_subscription_set_free(pubnub_subscription_set_t* set)
{
    if (NULL == set) { return false; }

    _subscription_set_reference_count_update(set, false);
    pubnub_mutex_lock(set->mutw);
    if (set->reference_count == 0) {
        if (NULL != set->subscriptions) { pbhash_set_free(set->subscriptions); }
        pubnub_mutex_unlock(set->mutw);
        pubnub_mutex_destroy(set->mutw);
        free(set);
        return true;
    }
    pubnub_mutex_unlock(set->mutw);
    return false;
}

void pubnub_subscribe_set_filter_expression(
    const pubnub_t* pb,
    const char*     filter_expr)
{
    if (NULL == pb) { return; }
    pbcc_subscribe_ee_set_filter_expression(pb->core.subscribe_ee, filter_expr);
}

void pubnub_subscribe_set_heartbeat(
    const pubnub_t* pb,
    const unsigned  heartbeat)
{
    if (NULL == pb) { return; }
    pbcc_subscribe_ee_set_heartbeat(pb->core.subscribe_ee, heartbeat);
}

pubnub_subscribe_cursor_t pubnub_subscribe_cursor(const char* timetoken)
{
    pubnub_subscribe_cursor_t cursor;
    if (NULL != timetoken) {
        memcpy(cursor.timetoken, timetoken, sizeof(cursor.timetoken) - 1);
        cursor.timetoken[sizeof(cursor.timetoken) - 1] = '\0';
    }
    else {
        cursor.timetoken[0] = '0';
        cursor.timetoken[1] = '\0';
    }
    cursor.region = 0;

    return cursor;
}

enum pubnub_res pubnub_subscribe_with_subscription(
    pubnub_subscription_t*           sub,
    const pubnub_subscribe_cursor_t* cursor)
{
    if (NULL == sub) { return PNR_INVALID_PARAMETERS; }

    const enum pubnub_res rslt = pbcc_subscribe_ee_subscribe_with_subscription(
        sub->ee,
        sub,
        cursor);

    if (PNR_OK == rslt) { _subscription_reference_count_update(sub, true); }

    return rslt;
}

enum pubnub_res pubnub_unsubscribe_with_subscription(
    pubnub_subscription_t* sub)
{
    if (NULL == sub) { return PNR_INVALID_PARAMETERS; }

    const enum pubnub_res rslt =
        pbcc_subscribe_ee_unsubscribe_with_subscription(sub->ee, sub);

    if (PNR_OK == rslt) { pubnub_subscription_free(sub); }

    return rslt;
}

enum pubnub_res pubnub_subscribe_with_subscription_set(
    pubnub_subscription_set_t*       set,
    const pubnub_subscribe_cursor_t* cursor)
{
    if (NULL == set) { return PNR_INVALID_PARAMETERS; }

    const enum pubnub_res rslt =
        pbcc_subscribe_ee_subscribe_with_subscription_set(
            set->ee,
            set,
            cursor);

    if (PNR_OK == rslt) {
        _subscription_set_reference_count_update(set, true);
        pubnub_mutex_lock(set->mutw);
        set->subscribed = true;
        pubnub_mutex_unlock(set->mutw);
    }

    return rslt;
}

enum pubnub_res pubnub_unsubscribe_with_subscription_set(
    pubnub_subscription_set_t* set)
{
    if (NULL == set) { return PNR_INVALID_PARAMETERS; }

    const enum pubnub_res rslt =
        pbcc_subscribe_ee_unsubscribe_with_subscription_set(set->ee, set);

    if (PNR_OK == rslt) {
        pubnub_mutex_lock(set->mutw);
        set->subscribed = false;
        pubnub_mutex_unlock(set->mutw);
        pubnub_subscription_set_free(set);
    }

    return rslt;
}

enum pubnub_res pubnub_disconnect(const pubnub_t* pb)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    return pbcc_subscribe_ee_disconnect(pb->core.subscribe_ee);
}

enum pubnub_res pubnub_reconnect(
    const pubnub_t*                 pb,
    const pubnub_subscribe_cursor_t cursor)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    return pbcc_subscribe_ee_reconnect(pb->core.subscribe_ee, cursor);
}

enum pubnub_res pubnub_unsubscribe_all(const pubnub_t* pb)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    return pbcc_subscribe_ee_unsubscribe_all(pb->core.subscribe_ee);
}

pubnub_subscription_t* _pubnub_subscription_alloc(
    pubnub_entity_t*                     entity,
    const pubnub_subscription_options_t* options,
    const bool                           standalone)
{
    PUBNUB_ASSERT_OPT(NULL != entity);
    PUBNUB_ASSERT(pb_valid_ctx_ptr(entity->pb));

    PBCC_ALLOCATE_TYPE(subscription, pubnub_subscription_t, true, NULL);
    pubnub_subscription_options_t opts = pubnub_subscription_options_defopts();
    if (NULL != options) {
        opts.receive_presence_events = options->receive_presence_events;
    }

    pubnub_mutex_init(subscription->mutw);
    subscription->options         = opts;
    subscription->reference_count = standalone == true ? 1 : 0;
    subscription->entity          = entity;
    subscription->ee              = entity->pb->core.subscribe_ee;

    return subscription;
}

void _subscription_reference_count_update(
    pubnub_subscription_t* subscription,
    const bool             increase)
{
    PUBNUB_ASSERT_OPT(NULL != subscription);

    pubnub_mutex_lock(subscription->mutw);
    if (increase) { subscription->reference_count++; }
    else if (subscription->reference_count > 0) {
        subscription->reference_count--;
    }
    pubnub_mutex_unlock(subscription->mutw);
}

pbhash_set_t* _pubnub_subscription_subscribables(
    const pubnub_subscription_t*         sub,
    const pubnub_subscription_options_t* options)
{
    pubnub_subscription_options_t opts = sub->options;
    if (NULL != options) {
        opts.receive_presence_events = sub->options.receive_presence_events;
    }

    const enum pubnub_subscribable_location location =
        sub->entity->type == PUBNUB_ENTITY_CHANNEL_GROUP
            ? SUBSCRIBABLE_LOCATION_QUERY
            : SUBSCRIBABLE_LOCATION_PATH;
    pbhash_set_t* hash = pbhash_set_alloc(
        opts.receive_presence_events ? 2 : 1,
        PBHASH_SET_CHAR_CONTENT_TYPE,
        NULL);
    if (NULL == hash) {
        PUBNUB_LOG_ERROR("pubnub_subscription_subscribables: failed to allocate"
                         " memory for subscription's subscribable list\n");
        return NULL;
    }

    pubnub_subscribable_t* regular = _pubnub_subscribable_alloc(
        location,
        &sub->entity->id,
        false);
    if (NULL == regular) {
        PUBNUB_LOG_ERROR("pubnub_subscription_subscribables: failed to allocate"
                         " memory for regular subscribable identifier\n");
        pbhash_set_free(hash);
        return NULL;
    }

    if (PBHSR_OUT_OF_MEMORY ==
        pbhash_set_add(hash, regular->id->ptr, regular)) {
        PUBNUB_LOG_ERROR(
            "pubnub_subscription_subscribables: failed to allocate memory "
            "for node to store regular subscribable identifier in hash set\n");
        _pubnub_subscribable_free(regular);
        pbhash_set_free(hash);
        return NULL;
    }

    if (!options->receive_presence_events) { return hash; }

    pubnub_subscribable_t* presence = _pubnub_subscribable_alloc(
        location,
        &sub->entity->id,
        true);
    if (NULL == presence) {
        PUBNUB_LOG_ERROR("pubnub_subscription_subscribables: failed to allocate"
                         " memory for presence subscribable identifier\n");
        pbhash_set_free_with_destructor(hash, _pubnub_subscribable_free);
        return NULL;
    }

    if (PBHSR_OUT_OF_MEMORY == pbhash_set_add(
            hash,
            presence->id->ptr,
            presence)) {
        PUBNUB_LOG_ERROR(
            "pubnub_subscription_subscribables: failed to allocate memory for "
            "node to store presence subscribable identifier in hash set\n");
        pbhash_set_free_with_destructor(hash, _pubnub_subscribable_free);
        _pubnub_subscribable_free(presence);
        return NULL;
    }

    return hash;
}

pubnub_subscription_set_t* _subscription_set_alloc(
    const pubnub_subscription_options_t* options,
    const int                            length)
{
    PBCC_ALLOCATE_TYPE(subscription_set, pubnub_subscription_set_t, true, NULL);
    pubnub_subscription_options_t opts = pubnub_subscription_options_defopts();
    if (NULL != options) {
        opts.receive_presence_events = options->receive_presence_events;
    }

    pubnub_mutex_init(subscription_set->mutw);
    subscription_set->options       = opts;
    subscription_set->subscriptions = pbhash_set_alloc(
        length,
        PBHASH_SET_CHAR_CONTENT_TYPE,
        pubnub_subscription_free);

    if (NULL == subscription_set->subscriptions) {
        PUBNUB_LOG_ERROR("subscription_set_alloc: failed to allocate memory for"
                         " subscriptions\n");
        pubnub_subscription_set_free(subscription_set);
        return NULL;
    }

    subscription_set->subscribed      = false;
    subscription_set->reference_count = 1;

    return subscription_set;
}

void _subscription_set_reference_count_update(
    pubnub_subscription_set_t* set,
    const bool                 increase)
{
    pubnub_mutex_lock(set->mutw);
    if (increase) { set->reference_count++; }
    else
        if (set->reference_count > 0) { set->reference_count--; }
    pubnub_mutex_unlock(set->mutw);
}

pbhash_set_t* _pubnub_subscription_set_subscribables(
    const pubnub_subscription_set_t* set)
{
    size_t                        count;
    const pubnub_subscription_t** subs = (const pubnub_subscription_t**)
        pbhash_set_elements(set->subscriptions, &count);

    if (NULL == subs) {
        PUBNUB_LOG_ERROR("pubnub_subscription_set_subscribables: failed to "
                         "allocate memory for subscriptions list\n");
    } else if (0 == count) {
        PUBNUB_LOG_INFO("pubnub_subscription_set_subscribables: subscriptions "
                        "list is empty");
    }

    pbhash_set_t* hash = pbhash_set_alloc(
        count * (set->options.receive_presence_events ? 2 : 1),
        PBHASH_SET_CHAR_CONTENT_TYPE,
        NULL);
    if (NULL == hash) {
        PUBNUB_LOG_ERROR("pubnub_subscription_set_get_subscribable: failed to "
                         "allocate memory for subscribables list\n");
        if (NULL != subs) { free(subs); }
        return NULL;
    }

    for (int i = 0; i < count; ++i) {
        pbhash_set_t* subsc = _pubnub_subscription_subscribables(
            subs[i],
            &set->options);

        if (NULL == subsc) {
            pbhash_set_free_with_destructor(hash, _pubnub_subscribable_free);
            free(subs);
            return NULL;
        }

        if (PBHSR_OUT_OF_MEMORY == pbhash_set_union(hash, subsc, NULL)) {
            PUBNUB_LOG_ERROR(
                "pubnub_subscription_set_get_subscribable: failed to allocate "
                "memory for nodes to store subscribable identifiers from other "
                "hash set\n");
            pbhash_set_free_with_destructor(hash, _pubnub_subscribable_free);
            pbhash_set_free_with_destructor(subsc, _pubnub_subscribable_free);
            free(subs);
            return NULL;
        }
    }
    if (NULL != subs) { free(subs); }

    return hash;
}

pubnub_subscribable_t* _pubnub_subscribable_alloc(
    const enum pubnub_subscribable_location location,
    struct pubnub_char_mem_block*           id,
    const bool                              presence)
{
    PBCC_ALLOCATE_TYPE(sub, pubnub_subscribable_t, true, NULL);
    sub->location              = location;
    sub->managed_memory_block  = !presence;
    if (!presence) { sub->id = id; }
    else {
        const char* suffix = "-pnpres";
        sub->id            = malloc(sizeof(struct pubnub_char_mem_block));
        sub->id->size      = id->size + strlen(suffix);
        sub->id->ptr       = malloc(sub->id->size + 1);
        sprintf(sub->id->ptr, "%s%s", id->ptr, suffix);
    }

    return sub;
}

size_t _pubnub_subscribable_length(const pubnub_subscribable_t* subscribable)
{
    return subscribable->id->size;
}

bool _pubnub_subscribable_is_presence(const pubnub_subscribable_t* subscribable)
{
    // Checking on variable which is set to `false` for presence because it is
    // allocated by the client itself.
    return !subscribable->managed_memory_block;
}

bool _pubnub_subscribable_is_cg(const pubnub_subscribable_t* subscribable)
{
    return subscribable->location == SUBSCRIBABLE_LOCATION_QUERY;
}

void _pubnub_subscribable_free(pubnub_subscribable_t* subscribable)
{
    if (NULL == subscribable) { return; }
    if (!subscribable->managed_memory_block) {
        if (subscribable->id->ptr) { free(subscribable->id->ptr); }
        free(subscribable->id);
    }

    free(subscribable);
}