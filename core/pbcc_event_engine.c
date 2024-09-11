#include "pbcc_event_engine.h"

#include <pubnub_mutex.h>
#include <stdlib.h>

#include "pbcc_memory_utils.h"
#include "core/pubnub_assert.h"
#include "lib/pbref_counter.h"
#include "core/pubnub_log.h"


// ----------------------------------------------
//                     Types
// ----------------------------------------------

// Sharable data type definition.
struct pbcc_ee_data {
    // Object references counter.
    pbref_counter_t* counter;
    /**
     * @brief Pointer to the data which should be associated with any of
     *        Event Engine objects: event, state, transition, or invocation.
     */
    const void* data;
    // Data destruct function.
    pbcc_ee_data_free_function_t data_free;
};

// Event Engine definition.
struct pbcc_event_engine {
    // Pointer to the current Event Engine state.
    pbcc_ee_state_t* current_state;
    /**
     * @brief Pointer to the FIFO list of invocations.
     *
     * @see pbcc_ee_invocation_t
     */
    pbarray_t* invocations;
    // Shared resources access lock.
    pubnub_mutex_t mutw;
};

// State transition instruction definition.
struct pbcc_ee_transition {
    /**
     * @brief Pointer to the next Event Engine state, which is based on current
     *        state and event passed into `transition` function.
     */
    pbcc_ee_state_t* target_state;
    /**
     * @brief Pointer to the list of effect invocations which should be called
     *        during the current Event Engine state change.
     */
    pbarray_t* invocations;
};

// Business logic invocation definition.
struct pbcc_ee_invocation {
    /**
     * @brief Whether invocation should be processed immediately without
     *        addition to the queue or not.
     */
    bool immediate;
    // Object references counter.
    pbref_counter_t* counter;
    /**
     * @brief Pointer to the additional information associated with a specific
     *        the Event Engine dispatcher into the effect execution function.
     */
    pbcc_ee_data_t* data;
    // State-specific effect function.
    pbcc_ee_effect_function_t effect;
};


// ----------------------------------------------
//              Function prototypes
// ----------------------------------------------

/**
 * @brief Create a copy of existing invocation.
 *
 * Copies share data and update reference counter to reflect active entities.
 *
 * @param invocation Pointer to the invocation object from which copy should be
 *                   created.
 * @return Pointer to the read to use effect invocation intention object or
 *         `NULL` in case of insufficient memory error. The returned pointer
 *         must be passed to the `pbcc_ee_invocation_free` to avoid a memory
 *         leak.
 */
static pbcc_ee_invocation_t* _pbcc_ee_invocation_copy(
    pbcc_ee_invocation_t* invocation);

/**
 * @brief Handle effect invocation completion.
 *
 * @param ee         Pointer to the event engine which enqueued effect
 *                   execution.
 * @param invocation Pointer to the invocation which has been used to execute
 *                   effect.
 */
static void _pbcc_ee_effect_completion(
    pbcc_event_engine_t*        ee,
    const pbcc_ee_invocation_t* invocation);


// ----------------------------------------------
//                   Functions
// ----------------------------------------------

pbcc_event_engine_t* pbcc_ee_alloc(pbcc_ee_state_t* initial_state)
{
    PUBNUB_ASSERT_OPT(NULL != initial_state);

    PBCC_ALLOCATE_TYPE(ee, pbcc_event_engine_t, true, NULL);
    ee->invocations = pbarray_alloc(5,
                                    PBARRAY_RESIZE_BALANCED,
                                    PBARRAY_GENERIC_CONTENT_TYPE,
                                    pbcc_ee_invocation_free);
    if (NULL == ee->invocations) {
        PUBNUB_LOG_ERROR("pbcc_ee_alloc: failed to allocate memory\n");
        pbcc_ee_free(ee);
        return NULL;
    }

    ee->current_state = initial_state;
    pubnub_mutex_init(ee->mutw);

    return ee;
}

pbcc_ee_state_t* pbcc_ee_current_state(const pbcc_event_engine_t* ee)
{
    pubnub_mutex_lock(ee->mutw);
    pbcc_ee_state_t* state = ee->current_state;
    pubnub_mutex_unlock(ee->mutw);

    return state;
}

enum pubnub_res pbcc_ee_handle_event(
    pbcc_event_engine_t* ee,
    pbcc_ee_event_t*     event)
{
    pubnub_mutex_lock(ee->mutw);
    PUBNUB_ASSERT_OPT(NULL != ee->current_state);

    const pbcc_ee_transition_t* transition = ee->current_state->transition(
        ee,
        ee->current_state,
        event);

    // Event not needed anymore.
    pbcc_ee_event_free(event);

    if (NULL == transition) {
        PUBNUB_LOG_ERROR("pbcc_ee_handle_event: failed to allocate memory\n");
        pubnub_mutex_unlock(ee->mutw);
        return PNR_OUT_OF_MEMORY;
    }

    // Check whether current state should be changed in response to `event`
    // or not.
    if (NULL == transition->target_state) {
        pubnub_mutex_unlock(ee->mutw);
        return PNR_OK;
    }
    pbcc_ee_state_t* previous_state = ee->current_state;
    ee->current_state               = transition->target_state;

    // Check whether any invocations expected to be called during transition
    // or not.
    if (NULL == transition->invocations) {
        pubnub_mutex_unlock(ee->mutw);
        return PNR_OK;
    }

    enum pubnub_res  rslt        = PNR_OK;
    const pbarray_t* invocations = transition->invocations;
    const size_t     count       = pbarray_count(invocations);
    for (size_t i = 0; i < count; ++i) {
        pbcc_ee_invocation_t* inv = _pbcc_ee_invocation_copy(
            (pbcc_ee_invocation_t*)pbarray_element_at(invocations, i));
        if (PBAR_OK != pbarray_add(ee->invocations,inv)) {
            pbcc_ee_invocation_free(inv);
            rslt = PNR_OUT_OF_MEMORY;
        }
    }

    // Restore previous state if new invocations schedule failed.
    if (PNR_OK != rslt) {
        for (size_t i = 0; i < count; ++i) {
            const pbcc_ee_invocation_t* inv =
                pbarray_element_at(invocations, i);
            pbarray_remove(ee->invocations, inv, false);
        }
        ee->current_state = previous_state;
    } else { pbcc_ee_state_free(previous_state); }

    for (size_t i = 0; i < count; ++i) {
        const pbcc_ee_invocation_t* inv = pbarray_element_at(invocations, i);
        if (inv->immediate)
            inv->effect(inv, inv->data->data, _pbcc_ee_effect_completion);
    }
    pubnub_mutex_unlock(ee->mutw);

    return rslt;
}

void pbcc_ee_process_next_invocation(
    pbcc_event_engine_t* ee,
    const bool           remove_previous)
{
    pubnub_mutex_lock(ee->mutw);
    if (remove_previous) { pbarray_remove_element_at(ee->invocations, 0); }

    const pbcc_ee_invocation_t* inv = pbarray_first(ee->invocations);
    if (NULL != inv)
        inv->effect(inv, inv->data->data, _pbcc_ee_effect_completion);
    pubnub_mutex_unlock(ee->mutw);
}

void pbcc_ee_free(pbcc_event_engine_t* ee)
{
    if (NULL == ee) { return; }

    pubnub_mutex_lock(ee->mutw);
    if (NULL != ee->current_state) { pbcc_ee_state_free(ee->current_state); }
    pubnub_mutex_unlock(ee->mutw);
    pubnub_mutex_destroy(ee->mutw);
    free(ee);
}

pbcc_ee_data_t* pbcc_ee_data_alloc(
    const void*                        data,
    const pbcc_ee_data_free_function_t data_free)
{
    PBCC_ALLOCATE_TYPE(ee_data, pbcc_ee_data_t, true, NULL);
    ee_data->counter = pbref_counter_alloc();
    if (NULL == ee_data->counter) {
        PUBNUB_LOG_ERROR("pbcc_ee_data_alloc: failed to allocate memory for "
            "reference counter\n");
        free(ee_data);
        return NULL;
    }

    ee_data->data      = data;
    ee_data->data_free = data_free;

    return ee_data;
}

const void* pbcc_ee_data_value(const pbcc_ee_data_t* data)
{
    return data->data;
}

pbcc_ee_data_t* pbcc_ee_data_copy(pbcc_ee_data_t* data)
{
    if (NULL == data) { return NULL; }
    pbref_counter_increment(data->counter);

    return data;
}

void pbcc_ee_data_free(pbcc_ee_data_t* data)
{
    if (NULL == data) { return; }

    if (0 == pbref_counter_free(data->counter)) {
        if (NULL != data->data_free) { data->data_free((void*)data->data); }
        free(data);
    }
}

pbcc_ee_state_t* pbcc_ee_state_alloc(const int type, pbcc_ee_data_t* data)
{
    PBCC_ALLOCATE_TYPE(state, pbcc_ee_state_t, true, NULL);
    state->type                 = type;
    state->data                 = pbcc_ee_data_copy(data);
    state->on_enter_invocations = NULL;
    state->on_exit_invocations  = NULL;
    state->transition           = NULL;

    return state;
}

void pbcc_ee_state_free(pbcc_ee_state_t* state)
{
    if (NULL == state) { return; }

    if (NULL != state->on_enter_invocations)
        pbarray_free(state->on_enter_invocations);
    if (NULL != state->on_exit_invocations)
        pbarray_free(state->on_exit_invocations);
    if (NULL != state->data) { pbcc_ee_data_free(state->data); }

    free(state);
}

enum pubnub_res pbcc_ee_state_add_on_enter_invocation(
    pbcc_ee_state_t*            state,
    const pbcc_ee_invocation_t* invocation)
{
    if (NULL == state->on_enter_invocations) {
        state->on_enter_invocations =
            pbarray_alloc(1,
                          PBARRAY_RESIZE_CONSERVATIVE,
                          PBARRAY_GENERIC_CONTENT_TYPE,
                          pbcc_ee_invocation_free);
        if (NULL == state->on_enter_invocations) {
            PUBNUB_LOG_ERROR("pbcc_ee_state_add_on_enter_invocation: failed to "
                "allocate memory for `on enter` invocations\n");
            return PNR_OUT_OF_MEMORY;
        }
    }

    const pbarray_res result = pbarray_add(state->on_enter_invocations,
                                           invocation);
    if (PBAR_OUT_OF_MEMORY == result) { return PNR_OUT_OF_MEMORY; }
    if (PBAR_INDEX_OUT_OF_RANGE == result) { return PNR_INVALID_PARAMETERS; }

    return PNR_OK;
}

enum pubnub_res pbcc_ee_state_add_on_exit_invocation(
    pbcc_ee_state_t*            state,
    const pbcc_ee_invocation_t* invocation)
{
    if (NULL == state->on_exit_invocations) {
        state->on_exit_invocations =
            pbarray_alloc(1,
                          PBARRAY_RESIZE_CONSERVATIVE,
                          PBARRAY_GENERIC_CONTENT_TYPE,
                          pbcc_ee_invocation_free);
        if (NULL == state->on_enter_invocations) {
            PUBNUB_LOG_ERROR("pbcc_ee_state_add_on_exit_invocation: failed to "
                "allocate memory for `on exit` invocations\n");
            return PNR_OUT_OF_MEMORY;
        }
    }

    const pbarray_res result = pbarray_add(state->on_exit_invocations,
                                           invocation);
    if (PBAR_OUT_OF_MEMORY == result) { return PNR_OUT_OF_MEMORY; }
    if (PBAR_INDEX_OUT_OF_RANGE == result) { return PNR_INVALID_PARAMETERS; }

    return PNR_OK;
}

pbcc_ee_event_t* pbcc_ee_event_alloc(const int type, pbcc_ee_data_t* data)
{
    PBCC_ALLOCATE_TYPE(event, pbcc_ee_event_t, true, NULL);
    event->type = type;
    event->data = pbcc_ee_data_copy(data);

    return event;
}

void pbcc_ee_event_free(pbcc_ee_event_t* event)
{
    if (NULL == event) { return; }
    if (NULL != event->data) { pbcc_ee_data_free(event->data); }
    free(event);
}

pbcc_ee_invocation_t* pbcc_ee_invocation_alloc(
    const pbcc_ee_effect_function_t effect,
    pbcc_ee_data_t*                 data)
{
    PBCC_ALLOCATE_TYPE(invocation, pbcc_ee_invocation_t, true, NULL);
    invocation->counter = pbref_counter_alloc();
    if (NULL == invocation->counter) {
        PUBNUB_LOG_ERROR("pbcc_ee_invocation_alloc: failed to allocate memory "
                         "for reference counter\n");
        free(invocation);
        return NULL;
    }
    pbref_counter_increment(invocation->counter);
    invocation->immediate = false;
    invocation->effect    = effect;
    invocation->data      = pbcc_ee_data_copy(data);

    return invocation;
}

pbcc_ee_invocation_t* _pbcc_ee_invocation_copy(
    pbcc_ee_invocation_t* invocation)
{
    if (NULL == invocation) { return NULL; }
    pbref_counter_increment(invocation->counter);

    return invocation;
}

void pbcc_ee_invocation_free(pbcc_ee_invocation_t* invocation)
{
    if (NULL == invocation) { return; }

    if (0 == pbref_counter_free(invocation->counter)) {
        if (NULL != invocation->data) { pbcc_ee_data_free(invocation->data); }
        free(invocation);
    }
}

pbcc_ee_transition_t* pbcc_ee_transition_alloc(
    const pbcc_event_engine_t* ee,
    pbcc_ee_state_t*           state,
    const pbarray_t*           invocations)
{
    PUBNUB_ASSERT_OPT(NULL != ee->current_state);

    const pbcc_ee_state_t* current_state = ee->current_state;
    PBCC_ALLOCATE_TYPE(transition, pbcc_ee_transition_t, true, NULL);
    transition->target_state = state;

    // Gather transition invocations list.
    size_t invocations_count = 0;
    if (NULL != current_state->on_exit_invocations)
        invocations_count += pbarray_count(current_state->on_exit_invocations);
    if (NULL != invocations)
        invocations_count += pbarray_count(invocations);
    if (NULL == state->on_enter_invocations)
        invocations_count += pbarray_count(state->on_enter_invocations);

    if (invocations_count > 0) {
        transition->invocations = pbarray_alloc(invocations_count,
                                                PBARRAY_RESIZE_NONE,
                                                PBARRAY_GENERIC_CONTENT_TYPE,
                                                pbcc_ee_invocation_free);

        if (NULL == transition->invocations) {
            PUBNUB_LOG_ERROR(
                "pbcc_ee_transition_alloc: failed to allocate memory "
                "for transition invocations\n");
            pbcc_ee_transition_free(transition);
            return NULL;
        }

        pbarray_merge(transition->invocations,
                      current_state->on_exit_invocations);
        pbarray_merge(transition->invocations, invocations);
        pbarray_merge(transition->invocations, state->on_enter_invocations);
    }
    else { transition->invocations = NULL; }

    return transition;
}

void pbcc_ee_transition_free(pbcc_ee_transition_t* transition)
{
    if (NULL == transition) { return; }

    pbcc_ee_state_free(transition->target_state);
    pbarray_free(transition->invocations);
    free(transition);
}

void _pbcc_ee_effect_completion(
    pbcc_event_engine_t*        ee,
    const pbcc_ee_invocation_t* invocation)
{
    pubnub_mutex_lock(ee->mutw);
    pbarray_remove(ee->invocations, invocation, true);
    pubnub_mutex_unlock(ee->mutw);
    pbcc_ee_process_next_invocation(ee, false);
}