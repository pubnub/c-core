/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#ifndef PBCC_EVENT_ENGINE_H
#define PBCC_EVENT_ENGINE_H


/**
 * @file  pbcc_event_engine.h
 * @brief Event Engine core implementation with base type definitions and
 *        functions.
 */

#include "core/pubnub_api_types.h"
#include "lib/pbarray.h"


// ----------------------------------------------
//               Types forwarding
// ----------------------------------------------

/** Event Engine definition. */
typedef struct pbcc_event_engine pbcc_event_engine_t;

/**
 * @brief State transition instruction definition.
 *
 * Transition contains information about expected target state and set of
 * invocations which should be performed during state change.
 */
typedef struct pbcc_ee_transition pbcc_ee_transition_t;

/**
 * @brief Business logic invocation definition.
 *
 * Invocation is intention to exec business logic and contains required data
 * for dispatcher to call actual implementation.
 */
typedef struct pbcc_ee_invocation pbcc_ee_invocation_t;

/** Event Engine state event forward declaration. */
typedef struct pbcc_ee_event pbcc_ee_event_t;

/** Event Engine state forward declaration. */
typedef struct pbcc_ee_state pbcc_ee_state_t;

/** User-provided data forward declaration. */
typedef struct pbcc_ee_data pbcc_ee_data_t;


// ----------------------------------------------
//                     Types
// ----------------------------------------------

/**
 * @brief Transition computation function definition.
 *
 * @param ee            Pointer to the Event Engine which will be requesting
 *                      for transition details in response to event.
 * @param current_state Pointer to the current Event Engine state.
 * @param event         Pointer to the Event Engine state changing event with
 *                      additional information required to decide on target
 *                      state.
 * @return Pointer to the transition for Event Engine current state change
 *         operation or `NULL` in case of insufficient memory error. The
 *         returned pointer must be passed to the `pbcc_ee_transition_free` to
 *         avoid a memory leak.
 */
typedef pbcc_ee_transition_t* (*pbcc_ee_transition_function_t)(
    pbcc_event_engine_t* ee,
    const pbcc_ee_state_t* current_state,
    const pbcc_ee_event_t* event);

/**
 * @brief User-provided data (`pbcc_ee_data_t`) destruction function
 *        definition.
 *
 * @param data Pointer to the user-provided data pointer, which should free up
 *             used resources.
 */
typedef void (*pbcc_ee_data_free_function_t)(void* data);

/**
 * @brief Effect invocation completion function definition.
 *
 * Function used by effect execution code to tell when required actions has been
 * done.
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
typedef void (*pbcc_ee_effect_completion_function_t)(
    pbcc_event_engine_t* ee,
    pbcc_ee_invocation_t* invocation,
    bool paused);

/**
 * @brief Invocation effect function definition.
 *
 * Function which is used by invocation dispatcher to perform business logic
 * for specific invocation.
 *
 * @param invocation Pointer to the invocation which has been used to execute
 *                   effect.
 * @param data       Pointer to the user-provided data, which will be passed
 *                   into function for processing.
 * @param cb         Pointer to the effect execution completion callback
 *                   function.
 * @return Effect execution result.
 */
typedef void (*pbcc_ee_effect_function_t)(
    const pbcc_ee_invocation_t* invocation,
    pbcc_ee_data_t* data,
    pbcc_ee_effect_completion_function_t cb);

/** Event Engine state event definition. */
struct pbcc_ee_event
{
    /** Specific Event Engine event type. */
    int type;
    /**
     * @brief Pointer to the additional information associated with a specific
     *        Event Engine event by event emitting code.
     */
    pbcc_ee_data_t* data;
};


// ----------------------------------------------
//                   Functions
// ----------------------------------------------

/**
 * @brief Create Event Engine with initial state.
 *
 * @param initial_state Pointer to the state from which all events handling and
 *                      transition should start.
 * @return Pointer to the ready to use Event Engine or `NULL` in case of
 *         insufficient memory error. The returned pointer must be passed to
 *         the `pbcc_ee_free` to avoid a memory leak.
 */
pbcc_event_engine_t* pbcc_ee_alloc(pbcc_ee_state_t* initial_state);

/**
 * @brief Clean up resources used by the Event Engine object.
 *
 * \b Important: Provided event engine pointer will be NULLified.
 *
 * @param ee Pointer to the Event Engine, which should free up used resources.
 */
void pbcc_ee_free(pbcc_event_engine_t** ee);

/**
 * @brief Retrieve copy of the current Event Engine state.
 *
 * @note Copies share state and update reference counter to reflect active
 *       entities.
 *
 * @param ee Pointer to the Event Engine for which current state should be
 *           returned.
 * @return The current state in which the Event Engine is at this moment. The
 *         returned pointer must be passed to the `pbcc_ee_state_free` to avoid
 *         a memory leak.
 *
 * @see pbcc_ee_state_free
 */
pbcc_ee_state_t* pbcc_ee_current_state(pbcc_event_engine_t* ee);

/**
 * @brief Handle new event for transition computation.
 *
 * @note Memory allocated for `event` will be managed by the Event Engine.
 *
 * @param ee    Pointer to the Event Engine which should process event.
 * @param event Pointer to the Event Engine state changing event object pointer
 *              which should be processed.
 * @return Result of event handling operation.
 */
enum pubnub_res pbcc_ee_handle_event(
    pbcc_event_engine_t* ee,
    pbcc_ee_event_t* event);

/**
 * @brief Process any scheduled invocations.
 *
 * @param ee              Pointer to the Event Engine which should dequeue
 *                        and process next invocation.
 * @return Number of enqueued effect invocations (ongoing and just created).
 */
size_t pbcc_ee_process_next_invocation(pbcc_event_engine_t* ee);

/**
 * @brief Handle effect invocation completion.
 *
 * @param ee         Pointer to the event engine which enqueued effect
 *                   execution.
 * @param invocation Pointer to the invocation which has been used to execute
 *                   effect.
 */
void pbcc_ee_handle_effect_completion(
    pbcc_event_engine_t* ee,
    pbcc_ee_invocation_t* invocation);

/**
 * @brief Create sharable user-provided data object.
 *
 * @note Memory allocated for `data` will be managed by the Event Engine if
 *       `data` destructor provided (`data_free`).
 *
 * @param data      Pointer to the data which will be shared between
 *                  different objects.
 * @param data_free User-provided data destruction function.
 * @return Pointer to the ready to use sharable Event Engine data or `NULL`
 *         in case of insufficient memory error. The returned pointer must be
 *         passed to the `pbcc_ee_data_free` to avoid a memory leak.
 */
pbcc_ee_data_t* pbcc_ee_data_alloc(
    void* data,
    pbcc_ee_data_free_function_t data_free);

/**
 * @brief Clean up resources used by the Event Engine shared data object.
 *
 * @param data Pointer to the Event Engine data, which should free up used
 *             resources.
 */
void pbcc_ee_data_free(pbcc_ee_data_t* data);

/**
 * @brief User-provided value.
 *
 * @param data Pointer to the data object from which user-provided value
 *             should be retrieved.
 * @return Pointer to the user-provided value.
 */
void* pbcc_ee_data_value(pbcc_ee_data_t* data);

/**
 * @brief Create a copy of existing data.
 *
 * @note Copies share data and update reference counter to reflect active
 *       entities.
 *
 * @param data Pointer to the data object from which copy should be created.
 * @return Pointer to the ready to use Event Engine data or `NULL` in case of
 *         insufficient memory error. The returned pointer must be passed to
 *         the `pbcc_ee_data_free` to avoid a memory leak.
 */
pbcc_ee_data_t* pbcc_ee_data_copy(pbcc_ee_data_t* data);

/**
 * @brief Create Event Engine state object.
 *
 * @note Memory allocated for `data` will be managed by the Event Engine if
 *       `data` destructor provided (`data_free`).
 *
 * @param type Specific Event Engine event type.
 * @param data Pointer to the additional information associated with a
 *             specific Event Engine event by event emitting code.
 * @param transition State-specific transition function to compute target state
 *                   and invocations to call.
 * @return Pointer to the Event Engine state.
 *
 * @see pbcc_ee_state_free
 */
pbcc_ee_state_t* pbcc_ee_state_alloc(
    int type,
    pbcc_ee_data_t* data,
    pbcc_ee_transition_function_t transition);

/**
 * @brief Clean up resources used by event engine state object.
 *
 * \b Important: Provided event engine state pointer will be NULLified.
 *
 * @param state Pointer to the state, which should free up resources.
 */
void pbcc_ee_state_free(pbcc_ee_state_t** state);

/**
 * @brief Get user-specified Event Engine state type.
 *
 * @note Safely, this function can be used only with state objects received from
 *       `pbcc_ee_current_state` function because the method returns shallow
 *       copy and there is no risk of segmentation fault error because of
 *       attempt to access freed memory.
 *
 * @param state Pointer to the state for which type should be retrieved.
 * @return User-specified state type.
 *
 * @see pbcc_ee_current_state
 */
int pbcc_ee_state_type(const pbcc_ee_state_t* state);

/**
 * @brief Get data associated with Event Engine state.
 *
 * @note Safely, this function can be used only with state objects received from
 *       `pbcc_ee_current_state` function because the method returns shallow
 *       copy and there is no risk of segmentation fault error because of
 *       attempt to access freed memory.
 *
 * @param state Pointer to the state for which associated data should be
 *              retrieved.
 * @return Data associated with state object or `NULL` in case if there is no
 *         data. The returned pointer must be passed to the `pbcc_ee_data_free`
 *         to avoid a memory leak.
 *
 * @see pbcc_ee_current_state
 */
const pbcc_ee_data_t* pbcc_ee_state_data(const pbcc_ee_state_t* state);

/**
 * @brief Add on state \b enter invocation.
 *
 * @note Memory allocated for `invocation` will be managed by the Event Engine.
 *
 * @param state      Pointer to the state, for which `on enter` invocation
 *                   will be added.
 * @param invocation Pointer to the invocation which should be called when
 *                   Event Engine will enter `state` as current state.<br/>
 *                   \b Important: Event Engine will manage `invocation`
 *                   memory after successful addition.
 * @return Result of `on enter` invocations list modification operation.
 */
enum pubnub_res pbcc_ee_state_add_on_enter_invocation(
    pbcc_ee_state_t* state,
    pbcc_ee_invocation_t* invocation);

/**
 * @brief Add on state \b exit invocation.
 *
 * @note Memory allocated for `invocation` will be managed by the Event Engine.
 *
 * @param state      Pointer to the state, for which `on exit` invocation
 *                   will be added.
 * @param invocation Pointer to the invocation which should be called when
 *                   Event Engine will leave `state` as current state before
 *                   entering new state.<br/>
 *                   \b Important: Event Engine will manage `invocation` memory
 *                   after successful addition.
 * @return Result of `on exit` invocations list modification operation.
 */
enum pubnub_res pbcc_ee_state_add_on_exit_invocation(
    pbcc_ee_state_t* state,
    pbcc_ee_invocation_t* invocation);

/**
 * @brief Create Event Engine state changing event.
 *
 * @note Memory allocated for `data` will be managed by the Event Engine if
 *       `data` destructor provided (`data_free`).
 *
 * @param type      Specific Event Engine event type.
 * @param data      Pointer to the additional information associated with a
 *                  specific Event Engine event by the event emitting code.
 * @return Pointer to ready to use event object or `NULL` in case of
 *         insufficient memory error. The returned pointer must be passed to
 *         the `pbcc_ee_event_free` to avoid a memory leak.
 */
pbcc_ee_event_t* pbcc_ee_event_alloc(int type, pbcc_ee_data_t* data);

/**
 * @brief Clean up resources used by the event object.
 *
 * \b Important: Provided event pointer will be NULLified.
 *
 * @param event     Pointer to the event object which, should free up used
 *                  resources.
 */
void pbcc_ee_event_free(pbcc_ee_event_t** event);

/**
 * @brief Create business logic invocation intent object.
 *
 * @note Memory allocated for `data` will be managed by the Event Engine if
 *       `data` destructor provided (`data_free`).
 *
 * @param type      A specific Event Engine invocation type.
 * @param effect    Function which will be called by dispatcher during
 *                  invocation handling.
 * @param data      Pointer to the data which has been associated by business
 *                  logic with an event and will be passed to the `effect`
 *                  function.
 * @param immediate Whether invocation should be processed immediately without
 *                  addition to the queue or not.
 * @return Pointer to the ready to use invocation object or `NULL` in case of
 *         insufficient memory error. The returned pointer must be passed to
 *         the `pbcc_ee_invocation_free` to avoid a memory leak.
 */
pbcc_ee_invocation_t* pbcc_ee_invocation_alloc(
    int type,
    pbcc_ee_effect_function_t effect,
    pbcc_ee_data_t* data,
    bool immediate);

/**
 * @brief Cancel next non-running invocation of specified type.
 *
 * The first invocation which is currently not active and matches the specified
 * type will be dequeued and freed.
 *
 * @param ee   Pointer to the Event Engine which enqueue scheduled invocations.
 * @param type A type of invocation which should be cancelled.
 */
void pbcc_ee_invocation_cancel_by_type(pbcc_event_engine_t* ee, int type);

/**
 * @brief Clean up resources used by the invocation object.
 *
 * @param invocation Pointer to the invocation, which should free up
 *                   resources.
 */
void pbcc_ee_invocation_free(pbcc_ee_invocation_t* invocation);

/**
 * @brief Create state transition instruction object.
 *
 * @note Memory allocated for `state` and `invocations` will be managed by
 *       the Event Engine.
 *
 * @param ee          Pointer to the Event Engine which will perform
 *                    transition.
 * @param state       Pointer to the target Event Engine state.
 * @param invocations Pointer to the list of invocations which should be done
 *                    before entering target state.
 * @return Pointer to the ready to use Event Engine state transition object or
 *         `NULL` in case of insufficient memory error. The returned pointer
 *         must be passed to the `pbcc_ee_transition_free` to avoid a memory
 *         leak.
 */
pbcc_ee_transition_t* pbcc_ee_transition_alloc(
    pbcc_event_engine_t* ee,
    pbcc_ee_state_t* state,
    pbarray_t* invocations);

/**
 * @brief Clean up resources used by the transition object.
 *
 * @note Function will `NULL`ify provided transition pointer.
 *
 * @param transition Pointer to the transition, which should free up
 *                   resources.
 */
void pbcc_ee_transition_free(pbcc_ee_transition_t** transition);
#endif // #ifndef PBCC_EVENT_ENGINE_H
