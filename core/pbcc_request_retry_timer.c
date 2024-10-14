/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbcc_request_retry_timer.h"

#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>

#include "lib/msstopwatch/msstopwatch.h"
#include "core/pbcc_memory_utils.h"
#include "core/pubnub_assert.h"
#include "core/pubnub_mutex.h"
#include "core/pb_sleep_ms.h"
#include "pubnub_internal.h"


// ----------------------------------------------
//                   Constants
// ----------------------------------------------

/**
 * @brief Polling timeout.
 *
 * Timeout used with sleep function inside of `while` loop to break constant
 * polling and give other processes some CPU time.
 */
#define PBCC_POLLING_TIMEOUT 10


// ----------------------------------------------
//                     Types
// ----------------------------------------------

#if defined      _WIN32
typedef DWORD    pubnub_thread_t;
typedef FILETIME pubnub_timespec_t;
typedef void     pubnub_watcher_t;
#define thread_handle_field() HANDLE thread_handle;
#define UNIT_IN_MILLI 1000
#else
typedef pthread_t       pubnub_thread_t;
typedef struct timespec pubnub_timespec_t;
typedef void*           pubnub_watcher_t;
#define thread_handle_field()
#endif

/** Failed requests retry timer definition. */
struct pbcc_request_retry_timer {
    /** PubNub context for which timer will restart requests. */
    pubnub_t* pb;
#if defined(PUBNUB_CALLBACK_API)
    /** Timer's run thread. */
    pubnub_thread_t timer_thread;
#endif // #if defined(PUBNUB_CALLBACK_API)
    /** Whether timer is running at this moment or not. */
    volatile bool running;
    /** Active timer delay value in milliseconds. */
    uint16_t delay;
    /** Shared resources access lock. */
    pubnub_mutex_t mutw;
};


// ----------------------------------------------
//              Function prototypes
// ----------------------------------------------


/**
 * @brief Run request retry timer.
 *
 * Launch timer which will await till timeout and try to restart request if
 * possible.
 *
 * @param timer Pointer to the timer which should run.
 */
static void* pbcc_request_retry_timer_run_(pbcc_request_retry_timer_t* timer);


// ----------------------------------------------
//                   Functions
// ----------------------------------------------

pbcc_request_retry_timer_t* pbcc_request_retry_timer_alloc(pubnub_t* pb)
{
    PBCC_ALLOCATE_TYPE(timer, pbcc_request_retry_timer_t, true, NULL);
    pubnub_mutex_init(timer->mutw);
#if defined(PUBNUB_CALLBACK_API)
    timer->timer_thread = NULL;
#endif // #if defined(PUBNUB_CALLBACK_API)
    timer->running = false;
    timer->pb      = pb;

    return timer;
}

void pbcc_request_retry_timer_free(pbcc_request_retry_timer_t** timer)
{
    if (NULL == timer || NULL == *timer) { return; }

    pbcc_request_retry_timer_stop(*timer);
    pubnub_mutex_destroy((*timer)->mutw);
    free(*timer);
    *timer = NULL;
}

void pbcc_request_retry_timer_start(
    pbcc_request_retry_timer_t* timer,
    const uint16_t              delay)
{
    if (NULL == timer || timer->running) { return; }
    PUBNUB_ASSERT(pb_valid_ctx_ptr(timer->pb));

    /** There shouldn't be any active timers. */
    pbcc_request_retry_timer_stop(timer);

    pubnub_mutex_lock(timer->pb->monitor);
    pubnub_mutex_lock(timer->mutw);
    timer->running = true;
    timer->delay   = delay;
    pubnub_mutex_unlock(timer->mutw);
    timer->pb->state = PBS_WAIT_RETRY;
    timer->pb->core.http_retry_count++;
    pubnub_mutex_unlock(timer->pb->monitor);

#if defined(PUBNUB_CALLBACK_API)
    if (pthread_create(&timer->timer_thread,
                       NULL,
                       (void*(*)(void*))pbcc_request_retry_timer_run_,
                       timer)) {
        PUBNUB_LOG_ERROR(
            "pbcc_request_retry_timer_start: unable to create thread");

        /**
         * To not stall FSM we let it retry request right away if thread can't
         * be created.
        */
        pubnub_mutex_lock(timer->mutw);
        timer->running = false;
        pubnub_mutex_unlock(timer->mutw);
        pubnub_mutex_lock(timer->pb->monitor);
        if (PBS_WAIT_RETRY == timer->pb->state) {
            timer->pb->state = PBS_RETRY;
            pbnc_fsm(timer->pb);
        }
        pubnub_mutex_unlock(timer->pb->monitor);
    }
#else
    pbcc_request_retry_timer_run_(timer);
#endif // #if defined(PUBNUB_CALLBACK_API)
}

void pbcc_request_retry_timer_stop(pbcc_request_retry_timer_t* timer)
{
    if (NULL == timer) { return; }

    pubnub_mutex_lock(timer->mutw);
#if defined(PUBNUB_CALLBACK_API)
    if (NULL != timer->timer_thread) {
        pthread_join(timer->timer_thread, NULL);
        timer->timer_thread = NULL;
    }
#endif // #if defined(PUBNUB_CALLBACK_API)
    timer->running = false;
    pubnub_mutex_unlock(timer->mutw);
}

void* pbcc_request_retry_timer_run_(pbcc_request_retry_timer_t* timer)
{
    const pbmsref_t t0      = pbms_start();
    pbms_t          delta   = 0;
    bool            stopped = false;

    pubnub_mutex_lock(timer->mutw);
    const uint16_t delay = timer->delay;
    pubnub_mutex_unlock(timer->mutw);
    while (delta < delay) {
        pubnub_mutex_lock(timer->mutw);
        if (!timer->running) {
            pubnub_mutex_unlock(timer->mutw);
            stopped = true;
            break;
        }
        pubnub_mutex_unlock(timer->mutw);

        delta = pbms_elapsed(t0);
        pb_sleep_ms(PBCC_POLLING_TIMEOUT);
    }

    pubnub_mutex_lock(timer->pb->monitor);
    if (!stopped && PBS_WAIT_RETRY == timer->pb->state) {
        timer->pb->state = PBS_RETRY;
        pbnc_fsm(timer->pb);
    }
    pubnub_mutex_unlock(timer->pb->monitor);
    pbcc_request_retry_timer_stop(timer);

    return NULL;
}