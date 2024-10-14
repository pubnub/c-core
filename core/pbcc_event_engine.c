/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbcc_event_engine.h"

#include <stdlib.h>

#include "core/pbcc_memory_utils.h"
#include "core/pubnub_assert.h"
#include "core/pubnub_mutex.h"
#include "lib/pbref_counter.h"
#include "core/pubnub_log.h"


// ----------------------------------------------
//                     Types
// ----------------------------------------------

/** Sharable data type definition. */
struct pbcc_ee_data {
    /** Object references counter. */
    pbref_counter_t* counter;
    /**
     * @brief Pointer to the data which should be associated with any of
     *        Event Engine objects: event, state, transition, or invocation.
     */
    void* data;
    /** Data destruct function. */
    pbcc_ee_data_free_function_t data_free;
};

/** Event Engine state definition. */
struct pbcc_ee_state {
    /** Object references counter. */
    pbref_counter_t* counter;
    /** A specific Event Engine state type. */
    int type;
    /**
     * @brief Pointer to the additional information associated with a specific
     *        Event Engine state.
     */
    pbcc_ee_data_t* data;
    /**
     * @brief Pointer to the invocations, which should be called when the
     *        Event Engine is about to enter a specific state.
     *
     * \b Important: `pbarray_t` should be created without elements destructor
     * to let `pbcc_ee_transition_t` manage invocation objects' life-cycle.
     */
    pbarray_t* on_enter_invocations;
    /**
     * @brief Pointer to the invocations, which should be called when the
     *        Event Engine is about to leave a specific state.
     *
     * \b Important: `pbarray_t` should be created without elements destructor
     * to let `pbcc_ee_transition_t` manage invocation objects' life-cycle.
     */
    pbarray_t* on_exit_invocations;
    /**
     * @brief State-specific transition function to compute target state and
     *        invocations to call.
     */
    pbcc_ee_transition_function_t transition;
};

/** Event Engine effect invocation status definition */
typedef enum {
    /** Effect invocation just has been created and not processed. */
    PBCC_EE_INVOCATION_CREATED,
    /** Invocation running is running effect implementation. */
    PBCC_EE_INVOCATION_RUNNING,
    /** Invocation completed effect execution. */
    PBCC_EE_INVOCATION_COMPLETED
} pbcc_ee_invocation_status;

/** Business logic invocation definition. */
struct pbcc_ee_invocation {
    /** A specific Event Engine invocation type. */
    int type;
    /**
     * @brief Whether invocation should be processed immediately without
     *        addition to the queue or not.
     */
    bool immediate;
    /** Current effect invocation status. */
    pbcc_ee_invocation_status status;
    /** Object references counter. */
    pbref_counter_t* counter;
    /**
     * @brief Pointer to the additional information associated with a specific
     *        the Event Engine dispatcher into the effect execution function.
     */
    pbcc_ee_data_t* data;
    /** State-specific effect function. */
    pbcc_ee_effect_function_t effect;
};

/** State transition instruction definition. */
struct pbcc_ee_transition {
    /**
     * @brief Pointer to the next Event Engine state, which is based on current
     *        state and event passed into `transition` function.
     */
    pbcc_ee_state_t* target_state;
    /**
     * @brief Pointer to the list of effect invocations which should be called
     *        during the current Event Engine state change.
     *
     * \b Important: Array allocated with `pbcc_ee_invocation_free` elements
     * destructor.
     */
    pbarray_t* invocations;
};

/** Event Engine definition. */
struct pbcc_event_engine {
    /** Pointer to the current Event Engine state. */
    pbcc_ee_state_t* current_state;
    /**
     * @brief Pointer to the FIFO list of invocations.
     *
     * \b Important: Array allocated with `pbcc_ee_invocation_free` elements
     * destructor.
     *
     * @see pbcc_ee_invocation_t
     */
    pbarray_t* invocations;
    /** Shared resources access lock. */
    pubnub_mutex_t mutw;
};


// ----------------------------------------------
//              Function prototypes
// ----------------------------------------------

/**
 * @brief Update current Event Engine state.
 *
 * @param ee    Pointer to the event engine for which state should be changed.
 * @param state Pointer to the state which should be set as current Event Engine
 *        state.
 */
static void pbcc_ee_current_state_set_(
    pbcc_event_engine_t* ee,
    pbcc_ee_state_t*     state);

/**
 * @brief Create a copy of existing state.
 *
 * @note Copies share state and update reference counter to reflect active
 *       entities.
 *
 * @param state Pointer to the state object from which copy should be created.
 * @return Pointer to the ready to use Event Engine state or `NULL` in case of
 *         insufficient memory error. The returned pointer must be passed to
 *         the `pbcc_ee_state_free` to avoid a memory leak.
 *
 * @see pbcc_ee_state_free
 */
static pbcc_ee_state_t* pbcc_ee_state_copy_(pbcc_ee_state_t* state);

/**
 * @brief Add invocation into invocations list.
 *
 * @param invocations Pointer to the array where new invocation should be added.
 * @param invocation  Pointer to the invocation which should be called when
 *                   Event Engine will leave `state` as current state before
 *                   entering new state.
 *                   <br/>\b Important: Event Engine will manage `invocation`
 *                   memory after successful addition.
 * @return Result of invocations list modification operation.
 */
static enum pubnub_res pbcc_ee_state_add_invocation_(
    pbarray_t**           invocations,
    pbcc_ee_invocation_t* invocation);

/**
 * @brief Update Event Engine state transition invocations list.
 *
 * @param transition  Pointer to the Event Engine state transition object for
 *                    which invocations list should be updated.
 * @param invocations Pointer to the list with invocation objects for
 *                    transition.
 * @return `false` if transition's invocations list modification failed.
 */
static bool pbcc_ee_transition_add_invocations_(
    const pbcc_ee_transition_t* transition,
    pbarray_t*                  invocations);

/**
 * @brief Execute effect invocation.
 *
 * @param invocation Pointer to the invocation which should execute effect.
 */
static void pbcc_ee_invocation_exec_(pbcc_ee_invocation_t* invocation);

/**
 * @brief Handle effect invocation completion.
 *
 * @param ee         Pointer to the event engine which enqueued effect
 *                   execution.
 * @param invocation Pointer to the invocation which has been used to execute
 *                   effect.
 * @param paused     Whether `invocation` execution has been paused or not.
 *                   Paused invocation will reset its execution status to
 *                   'CREATED'. When paused, then the Event Engine should be
 *                   told when the next invocation should be processed.
 */
static void pbcc_ee_effect_completion_(
    pbcc_event_engine_t*  ee,
    pbcc_ee_invocation_t* invocation,
    bool                  paused);


// ----------------------------------------------
//                   Functions
// ----------------------------------------------

pbcc_event_engine_t* pbcc_ee_alloc(pbcc_ee_state_t* initial_state)
{
    PUBNUB_ASSERT_OPT(NULL != initial_state);

    PBCC_ALLOCATE_TYPE(ee, pbcc_event_engine_t, true, NULL);
    ee->current_state = initial_state;
    pubnub_mutex_init(ee->mutw);

    ee->invocations = pbarray_alloc(5,
                                    PBARRAY_RESIZE_BALANCED,
                                    PBARRAY_GENERIC_CONTENT_TYPE,
                                    (pbarray_element_free)
                                    pbcc_ee_invocation_free);
    if (NULL == ee->invocations) {
        PUBNUB_LOG_ERROR("pbcc_ee_alloc: failed to allocate memory\n");
        pbcc_ee_free(&ee);
        return NULL;
    }

    return ee;
}

void pbcc_ee_free(pbcc_event_engine_t** ee)
{
    if (NULL == ee || NULL == *ee) { return; }

    pubnub_mutex_lock((*ee)->mutw);
    if (NULL != (*ee)->invocations) { pbarray_free(&(*ee)->invocations); }
    if (NULL != (*ee)->current_state)
        pbcc_ee_state_free(&(*ee)->current_state);
    pubnub_mutex_unlock((*ee)->mutw);
    pubnub_mutex_destroy((*ee)->mutw);
    free(*ee);
    *ee = NULL;
}

pbcc_ee_state_t* pbcc_ee_current_state(pbcc_event_engine_t* ee)
{
    pubnub_mutex_lock(ee->mutw);
    pbcc_ee_state_t* state = pbcc_ee_state_copy_(ee->current_state);
    pubnub_mutex_unlock(ee->mutw);

    return state;
}

enum pubnub_res pbcc_ee_handle_event(
    pbcc_event_engine_t* ee,
    pbcc_ee_event_t*     event)
{
    PUBNUB_ASSERT_OPT(NULL != ee->current_state);

    pubnub_mutex_lock(ee->mutw);
    pbcc_ee_state_t*      state      = pbcc_ee_state_copy_(ee->current_state);
    pbcc_ee_transition_t* transition = ee->current_state->transition(
        ee,
        state,
        event);
    pbcc_ee_state_free(&state);
    pbcc_ee_event_free(&event);

    if (NULL == transition) {
        PUBNUB_LOG_ERROR("pbcc_ee_handle_event: failed to allocate memory\n");
        pubnub_mutex_unlock(ee->mutw);
        return PNR_OUT_OF_MEMORY;
    }

    /**
     * Check whether current state should be changed in response to `event`
     * or not. This is special case of transition to mark that there were no
     * error during `transition` function call.
     */
    if (NULL == transition->target_state) {
        pbcc_ee_transition_free(&transition);
        pubnub_mutex_unlock(ee->mutw);
        return PNR_OK;
    }

    /**
     * Check whether any invocations expected to be called during transition
     * or not.
     */
    if (NULL == transition->invocations) {
        pbcc_ee_current_state_set_(ee, transition->target_state);
        pbcc_ee_transition_free(&transition);
        pubnub_mutex_unlock(ee->mutw);

        return PNR_OK;
    }

    /**
     * Merging transition invocations into Event Engine invocations queue.
     */
    enum pubnub_res   rslt        = PNR_OK;
    pbarray_t*        invocations = transition->invocations;
    const size_t      count       = pbarray_count(invocations);
    const pbarray_res merge_rslt  = pbarray_merge(ee->invocations, invocations);
    if (PBAR_OK != merge_rslt) {
        rslt = PBTT_ADD_MEMBERS == merge_rslt
                   ? PNR_OUT_OF_MEMORY
                   : PNR_INVALID_PARAMETERS;
    }

    if (PNR_OK != rslt || 0 == count) {
        if (0 != count)
            pbarray_subtract(ee->invocations, invocations, true);
        if (PNR_OK == rslt)
            pbcc_ee_current_state_set_(ee, transition->target_state);
        pubnub_mutex_unlock(ee->mutw);

        pbcc_ee_transition_free(&transition);
        pbcc_ee_process_next_invocation(ee);

        return PNR_OK;
    }

    pbcc_ee_current_state_set_(ee, transition->target_state);
    pubnub_mutex_unlock(ee->mutw);

    for (size_t i = 0; i < count; ++i) {
        pbcc_ee_invocation_t* inv = (pbcc_ee_invocation_t*)
            pbarray_element_at(invocations, i);
        /**
         * Make sure that invocation has proper ref count after addition to the
         * Event Engine invocations queue from transition.
         */
        pbref_counter_increment(inv->counter);
    }

    /** Check whether any immediate  */
    pbcc_ee_invocation_t* inv =
        (pbcc_ee_invocation_t*)pbarray_first(invocations);
    if (inv->immediate) { pbcc_ee_invocation_exec_(inv); }
    else { pbcc_ee_process_next_invocation(ee); }

    pbcc_ee_transition_free(&transition);

    return rslt;
}

size_t pbcc_ee_process_next_invocation(pbcc_event_engine_t* ee)
{
    pubnub_mutex_lock(ee->mutw);
    const size_t count = pbarray_count(ee->invocations);

    if (0 == count) {
        pubnub_mutex_unlock(ee->mutw);
        return count;
    }

    pbcc_ee_invocation_t* invocation = (pbcc_ee_invocation_t*)
        pbarray_first(ee->invocations);
    pubnub_mutex_unlock(ee->mutw);
    pbcc_ee_invocation_exec_(invocation);

    return count;
}

void pbcc_ee_handle_effect_completion(
    pbcc_event_engine_t*  ee,
    pbcc_ee_invocation_t* invocation)
{
    /** Check whether invocation is running or not. */
    if (PBCC_EE_INVOCATION_RUNNING != invocation->status) { return; }

    pubnub_mutex_lock(ee->mutw);
    invocation->status = PBCC_EE_INVOCATION_COMPLETED;
    pbarray_remove(ee->invocations, (void**)&invocation, true);
    pubnub_mutex_unlock(ee->mutw);
}

pbcc_ee_data_t* pbcc_ee_data_alloc(
    void*                              data,
    const pbcc_ee_data_free_function_t data_free)
{
    PBCC_ALLOCATE_TYPE(ee_data, pbcc_ee_data_t, true, NULL);
    ee_data->counter   = pbref_counter_alloc();
    ee_data->data      = data;
    ee_data->data_free = data_free;

    return ee_data;
}

void pbcc_ee_data_free(pbcc_ee_data_t* data)
{
    if (NULL == data) { return; }

    if (0 == pbref_counter_free(data->counter)) {
        if (NULL != data->data_free) { data->data_free(data->data); }
        free(data);
    }
}

void* pbcc_ee_data_value(pbcc_ee_data_t* data)
{
    if (NULL == data) { return NULL; }
    return data->data;
}

pbcc_ee_data_t* pbcc_ee_data_copy(pbcc_ee_data_t* data)
{
    if (NULL == data) { return NULL; }

    pbref_counter_increment(data->counter);

    return data;
}

pbcc_ee_state_t* pbcc_ee_state_alloc(
    const int                           type,
    pbcc_ee_data_t*                     data,
    const pbcc_ee_transition_function_t transition)
{
    PBCC_ALLOCATE_TYPE(state, pbcc_ee_state_t, true, NULL);
    state->counter              = pbref_counter_alloc();
    state->type                 = type;
    state->data                 = pbcc_ee_data_copy(data);
    state->on_enter_invocations = NULL;
    state->on_exit_invocations  = NULL;
    state->transition           = transition;

    return state;
}

void pbcc_ee_state_free(pbcc_ee_state_t** state)
{
    if (NULL == state || NULL == *state) { return; }

    if (0 == pbref_counter_free((*state)->counter)) {
        if (NULL != (*state)->on_enter_invocations)
            pbarray_free(&(*state)->on_enter_invocations);
        if (NULL != (*state)->on_exit_invocations)
            pbarray_free(&(*state)->on_exit_invocations);
        if (NULL != (*state)->data) { pbcc_ee_data_free((*state)->data); }

        free(*state);
        *state = NULL;
    }
}

int pbcc_ee_state_type(const pbcc_ee_state_t* state)
{
    return state->type;
}

const pbcc_ee_data_t* pbcc_ee_state_data(const pbcc_ee_state_t* state)
{
    if (NULL == state->data) { return NULL; }
    return pbcc_ee_data_copy(state->data);
}

enum pubnub_res pbcc_ee_state_add_on_enter_invocation(
    pbcc_ee_state_t*      state,
    pbcc_ee_invocation_t* invocation)
{
    return pbcc_ee_state_add_invocation_(&state->on_enter_invocations,
                                         invocation);
}

enum pubnub_res pbcc_ee_state_add_on_exit_invocation(
    pbcc_ee_state_t*      state,
    pbcc_ee_invocation_t* invocation)
{
    return pbcc_ee_state_add_invocation_(&state->on_exit_invocations,
                                         invocation);
}

pbcc_ee_event_t* pbcc_ee_event_alloc(const int type, pbcc_ee_data_t* data)
{
    PBCC_ALLOCATE_TYPE(event, pbcc_ee_event_t, true, NULL);
    event->type = type;
    event->data = data;

    return event;
}

void pbcc_ee_event_free(pbcc_ee_event_t** event)
{
    if (NULL == event || NULL == *event) { return; }

    if (NULL != (*event)->data) { pbcc_ee_data_free((*event)->data); }
    free(*event);
    *event = NULL;
}

pbcc_ee_invocation_t* pbcc_ee_invocation_alloc(
    int                             type,
    const pbcc_ee_effect_function_t effect,
    pbcc_ee_data_t*                 data,
    const bool                      immediate)
{
    PBCC_ALLOCATE_TYPE(invocation, pbcc_ee_invocation_t, true, NULL);
    invocation->immediate = immediate;
    invocation->counter   = pbref_counter_alloc();
    invocation->status    = PBCC_EE_INVOCATION_CREATED;
    invocation->effect    = effect;
    invocation->data      = pbcc_ee_data_copy(data);
    invocation->type      = type;

    return invocation;
}

void pbcc_ee_invocation_cancel_by_type(pbcc_event_engine_t* ee, int type)
{
    pubnub_mutex_lock(ee->mutw);
    const size_t count = pbarray_count(ee->invocations);
    if (0 == count) {
        pubnub_mutex_unlock(ee->mutw);
        return;
    }

    for (size_t i = 0; i < count; i++) {
        const pbcc_ee_invocation_t* invocation =
            pbarray_element_at(ee->invocations, i);

        if (invocation->status == PBCC_EE_INVOCATION_RUNNING) { continue; }
        if (type == invocation->type) {
            pbarray_remove(ee->invocations, (void**)&invocation, false);
            break;
        }
    }
    pubnub_mutex_unlock(ee->mutw);
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
    pbcc_event_engine_t* ee,
    pbcc_ee_state_t*     state,
    pbarray_t*           invocations)
{
    PUBNUB_ASSERT_OPT(NULL != ee->current_state);

    pubnub_mutex_lock(ee->mutw);
    const pbcc_ee_state_t* current_state = ee->current_state;
    PBCC_ALLOCATE_TYPE(transition, pbcc_ee_transition_t, true, NULL);
    transition->target_state = pbcc_ee_state_copy_(state);

    /** Gather transition invocations list. */
    size_t invocations_count = 0;
    if (NULL != state && NULL != current_state->on_exit_invocations)
        invocations_count += pbarray_count(current_state->on_exit_invocations);
    if (NULL != state && NULL != invocations)
        invocations_count += pbarray_count(invocations);
    if (NULL != state && NULL != state->on_enter_invocations)
        invocations_count += pbarray_count(state->on_enter_invocations);

    if (invocations_count > 0) {
        transition->invocations = pbarray_alloc(invocations_count,
                                                PBARRAY_RESIZE_NONE,
                                                PBARRAY_GENERIC_CONTENT_TYPE,
                                                (pbarray_element_free)
                                                pbcc_ee_invocation_free);

        pbarray_t* on_exit  = current_state->on_exit_invocations;
        pbarray_t* on_enter =
            NULL != state ? state->on_enter_invocations : NULL;

        /**
         * Attention: `invocations` should be called before `on_exit` because
         * in most of the cases `on_exit` cancels previous request and it will
         * reset all read buffers making it impossible to process data received
         * for current state.
         */
        if (NULL == transition->invocations ||
            !pbcc_ee_transition_add_invocations_(transition, invocations) ||
            !pbcc_ee_transition_add_invocations_(transition, on_exit) ||
            !pbcc_ee_transition_add_invocations_(transition, on_enter)) {
            PUBNUB_LOG_ERROR(
                "pbcc_ee_transition_alloc: failed to allocate memory "
                "for transition invocations\n");
            if (NULL != invocations) { pbarray_free(&invocations); }
            pbcc_ee_transition_free(&transition);
            pubnub_mutex_unlock(ee->mutw);

            return NULL;
        }
    }
    else { transition->invocations = NULL; }
    if (NULL != invocations) { pbarray_free(&invocations); }
    pubnub_mutex_unlock(ee->mutw);

    return transition;
}

void pbcc_ee_transition_free(pbcc_ee_transition_t** transition)
{
    if (NULL == transition || NULL == *transition) { return; }

    pbcc_ee_state_free(&(*transition)->target_state);
    if (NULL != (*transition)->invocations)
        pbarray_free(&(*transition)->invocations);
    free(*transition);
    *transition = NULL;
}

void pbcc_ee_current_state_set_(pbcc_event_engine_t* ee, pbcc_ee_state_t* state)
{
    pbcc_ee_state_free(&ee->current_state);
    ee->current_state = pbcc_ee_state_copy_(state);
}

pbcc_ee_state_t* pbcc_ee_state_copy_(pbcc_ee_state_t* state)
{
    if (NULL == state) { return NULL; }

    /**
     * Copying is done by making sure that an instance won't be freed until last
     * reference to it will be freed.
     */
    pbref_counter_increment(state->counter);

    return state;
}

enum pubnub_res pbcc_ee_state_add_invocation_(
    pbarray_t**           invocations,
    pbcc_ee_invocation_t* invocation)
{
    if (NULL == *invocations) {
        *invocations = pbarray_alloc(1,
                                     PBARRAY_RESIZE_CONSERVATIVE,
                                     PBARRAY_GENERIC_CONTENT_TYPE,
                                     (pbarray_element_free)
                                     pbcc_ee_invocation_free);
        if (NULL == *invocations) {
            PUBNUB_LOG_ERROR("pbcc_ee_state_add_invocation: failed to allocate "
                "memory for invocations\n");
            return PNR_OUT_OF_MEMORY;
        }
    }

    const pbarray_res result = pbarray_add(*invocations, invocation);
    if (PBAR_OUT_OF_MEMORY == result) { return PNR_OUT_OF_MEMORY; }
    if (PBAR_INDEX_OUT_OF_RANGE == result) { return PNR_INVALID_PARAMETERS; }

    return PNR_OK;
}

bool pbcc_ee_transition_add_invocations_(
    const pbcc_ee_transition_t* transition,
    pbarray_t*                  invocations)
{
    if (NULL == invocations) { return true; }
    if (PBAR_OK != pbarray_merge(transition->invocations, invocations))
        return false;

    const size_t count = pbarray_count(invocations);
    for (size_t i = 0; i < count; i++) {
        const pbcc_ee_invocation_t* invocation =
            pbarray_element_at(invocations, i);
        pbref_counter_increment(invocation->counter);
    }

    return true;
}

void pbcc_ee_invocation_exec_(pbcc_ee_invocation_t* invocation)
{
    /** Check whether invocation still runnable or not. */
    if (PBCC_EE_INVOCATION_CREATED != invocation->status) { return; }

    invocation->status = PBCC_EE_INVOCATION_RUNNING;
    invocation->effect(
        invocation,
        invocation->data,
        (pbcc_ee_effect_completion_function_t)pbcc_ee_effect_completion_);
}

void pbcc_ee_effect_completion_(
    pbcc_event_engine_t*  ee,
    pbcc_ee_invocation_t* invocation,
    const bool            paused)
{
    /** Check whether invocation is running or not. */
    if (PBCC_EE_INVOCATION_RUNNING != invocation->status) { return; }

    /** Check whether called effect asked to pospone its execution or not. */
    if (paused) {
        invocation->status = PBCC_EE_INVOCATION_CREATED;
        return;
    }

    pbcc_ee_handle_effect_completion(ee, invocation);
    pbcc_ee_process_next_invocation(ee);
}