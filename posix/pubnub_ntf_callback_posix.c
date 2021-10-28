/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "core/pubnub_ntf_callback.h"

#include "posix/monotonic_clock_get_time.h"
#include "posix/pbtimespec_elapsed_ms.h"

#include "pubnub_internal.h"
#include "core/pubnub_assert.h"
#include "core/pubnub_log.h"
#include "core/pubnub_timer_list.h"
#include "core/pbpal.h"

#include "lib/sockets/pbpal_ntf_callback_poller_poll.h"
#include "core/pbpal_ntf_callback_queue.h"
#include "core/pbpal_ntf_callback_handle_timer_list.h"

#include <pthread.h>

#include <stdlib.h>
#include <string.h>


struct SocketWatcherData {
    struct pbpal_poll_data* poll pubnub_guarded_by(mutw);
    bool stop_socket_watcher_thread pubnub_guarded_by(stoplock);
    pthread_mutex_t mutw;
    pthread_mutex_t timerlock;
    pthread_mutex_t stoplock;
    pthread_t       thread_id;
#if PUBNUB_TIMERS_API
    pubnub_t* timer_head pubnub_guarded_by(timerlock);
#endif
    struct pbpal_ntf_callback_queue queue;
};


static struct SocketWatcherData m_watcher;


int pbntf_watch_in_events(pubnub_t* pbp)
{
    return pbpal_ntf_watch_in_events(m_watcher.poll, pbp);
}


int pbntf_watch_out_events(pubnub_t* pbp)
{
    return pbpal_ntf_watch_out_events(m_watcher.poll, pbp);
}


void* socket_watcher_thread(void* arg)
{
    const int max_poll_ms = 100;
    struct timespec prev_timspec;
    monotonic_clock_get_time(&prev_timspec);

    for (;;) {
        struct timespec timspec;
        bool stop_thread;
        
        pthread_mutex_lock(&m_watcher.stoplock);
        stop_thread = m_watcher.stop_socket_watcher_thread;
        pthread_mutex_unlock(&m_watcher.stoplock);
        if (stop_thread) {
            break;
        }
        
        pbpal_ntf_callback_process_queue(&m_watcher.queue);

        monotonic_clock_get_time(&timspec);

        pthread_mutex_lock(&m_watcher.mutw);
        pbpal_ntf_poll_away(m_watcher.poll, max_poll_ms);
        pthread_mutex_unlock(&m_watcher.mutw);

        if (PUBNUB_TIMERS_API) {
            int elapsed = pbtimespec_elapsed_ms(prev_timspec, timspec);
            if (elapsed > 0) {
                if (elapsed > max_poll_ms + 5) {
                    PUBNUB_LOG_TRACE("elapsed = %d: prev_timspec={%ld, %ld}, timspec={%ld,%ld}\n",
                                     elapsed,
                                     prev_timspec.tv_sec, prev_timspec.tv_nsec,
                                     timspec.tv_sec, timspec.tv_nsec
                                     
                        );
                }
                pthread_mutex_lock(&m_watcher.timerlock);
                pbntf_handle_timer_list(elapsed, &m_watcher.timer_head);
                pthread_mutex_unlock(&m_watcher.timerlock);

                prev_timspec = timspec;
            }
        }
    }

    return NULL;
}


void pubnub_stop(void)
{
    pbauto_heartbeat_stop();
    
    pthread_mutex_lock(&m_watcher.stoplock);
    m_watcher.stop_socket_watcher_thread = true;
    pthread_mutex_unlock(&m_watcher.stoplock);
}
    

int pbntf_init(void)
{
    int                 rslt;
    pthread_mutexattr_t attr;

    rslt = pthread_mutexattr_init(&attr);
    if (rslt != 0) {
        PUBNUB_LOG_ERROR(
            "Failed to initialize mutex attributes, error code: %d", rslt);
        return -1;
    }
    rslt = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (rslt != 0) {
        PUBNUB_LOG_ERROR("Failed to set mutex attribute type, error code: %d",
                         rslt);
        pthread_mutexattr_destroy(&attr);
        return -1;
    }
    rslt = pthread_mutex_init(&m_watcher.stoplock, &attr);
    if (rslt != 0) {
        PUBNUB_LOG_ERROR("Failed to initialize 'stoplock' mutex, error code: %d", rslt);
        pthread_mutexattr_destroy(&attr);
        return -1;
    }
    rslt = pthread_mutex_init(&m_watcher.mutw, &attr);
    if (rslt != 0) {
        PUBNUB_LOG_ERROR("Failed to initialize mutex, error code: %d", rslt);
        pthread_mutexattr_destroy(&attr);
        pthread_mutex_destroy(&m_watcher.stoplock);
        return -1;
    }
    rslt = pthread_mutex_init(&m_watcher.timerlock, &attr);
    if (rslt != 0) {
        PUBNUB_LOG_ERROR("Failed to initialize mutex for timers, error code: %d", rslt);
        pthread_mutexattr_destroy(&attr);
        pthread_mutex_destroy(&m_watcher.mutw);
        pthread_mutex_destroy(&m_watcher.stoplock);
        return -1;
    }

    m_watcher.poll = pbpal_ntf_callback_poller_init();
    if (NULL == m_watcher.poll) {
        pthread_mutexattr_destroy(&attr);
        pthread_mutex_destroy(&m_watcher.mutw);
        pthread_mutex_destroy(&m_watcher.timerlock);
        pthread_mutex_destroy(&m_watcher.stoplock);
        return -1;
    }
    pbpal_ntf_callback_queue_init(&m_watcher.queue);
    m_watcher.stop_socket_watcher_thread = false;

#if defined(PUBNUB_CALLBACK_THREAD_STACK_SIZE_KB)                              \
    && (PUBNUB_CALLBACK_THREAD_STACK_SIZE_KB > 0)
    {
        pthread_attr_t thread_attr;

        rslt = pthread_attr_init(&thread_attr);
        if (rslt != 0) {
            PUBNUB_LOG_ERROR(
                "Failed to initialize thread attributes, error code: %d\n", rslt);
            pthread_mutexattr_destroy(&attr);
            pthread_mutex_destroy(&m_watcher.mutw);
            pthread_mutex_destroy(&m_watcher.timerlock);
            pthread_mutex_destroy(&m_watcher.stoplock);
            pbpal_ntf_callback_queue_deinit(&m_watcher.queue);
            pbpal_ntf_callback_poller_deinit(&m_watcher.poll);
            return -1;
        }
        rslt = pthread_attr_setstacksize(
            &thread_attr, PUBNUB_CALLBACK_THREAD_STACK_SIZE_KB * 1024);
        if (rslt != 0) {
            PUBNUB_LOG_ERROR(
                "Failed to set thread stack size to %d kb, error code: %d\n",
                PUBNUB_CALLBACK_THREAD_STACK_SIZE_KB,
                rslt);
            pthread_mutexattr_destroy(&attr);
            pthread_mutex_destroy(&m_watcher.mutw);
            pthread_mutex_destroy(&m_watcher.timerlock);
            pthread_mutex_destroy(&m_watcher.stoplock);
            pthread_attr_destroy(&thread_attr);
            pbpal_ntf_callback_queue_deinit(&m_watcher.queue);
            pbpal_ntf_callback_poller_deinit(&m_watcher.poll);
            return -1;
        }
        rslt = pthread_create(
            &m_watcher.thread_id, &thread_attr, socket_watcher_thread, NULL);
        if (rslt != 0) {
            PUBNUB_LOG_ERROR(
                "Failed to create the polling thread, error code: %d\n", rslt);
            pthread_mutexattr_destroy(&attr);
            pthread_mutex_destroy(&m_watcher.mutw);
            pthread_mutex_destroy(&m_watcher.timerlock);
            pthread_mutex_destroy(&m_watcher.stoplock);
            pthread_attr_destroy(&thread_attr);
            pbpal_ntf_callback_queue_deinit(&m_watcher.queue);
            pbpal_ntf_callback_poller_deinit(&m_watcher.poll);
            return -1;
        }
    }
#else
    rslt =
        pthread_create(&m_watcher.thread_id, NULL, socket_watcher_thread, NULL);
    if (rslt != 0) {
        PUBNUB_LOG_ERROR(
            "Failed to create the polling thread, error code: %d\n", rslt);
        pthread_mutexattr_destroy(&attr);
        pthread_mutex_destroy(&m_watcher.mutw);
        pthread_mutex_destroy(&m_watcher.timerlock);
        pthread_mutex_destroy(&m_watcher.stoplock);
        pbpal_ntf_callback_queue_deinit(&m_watcher.queue);
        pbpal_ntf_callback_poller_deinit(&m_watcher.poll);
        return -1;
    }
#endif

    return 0;
}


int pbntf_enqueue_for_processing(pubnub_t* pb)
{
    return pbpal_ntf_callback_enqueue_for_processing(&m_watcher.queue, pb);
}


int pbntf_requeue_for_processing(pubnub_t* pb)
{
    return pbpal_ntf_callback_requeue_for_processing(&m_watcher.queue, pb);
}


int pbntf_got_socket(pubnub_t* pb)
{
    pthread_mutex_lock(&m_watcher.mutw);
    pbpal_ntf_callback_save_socket(m_watcher.poll, pb);
    pthread_mutex_unlock(&m_watcher.mutw);

    if (PUBNUB_TIMERS_API) {
        pthread_mutex_lock(&m_watcher.timerlock);
        m_watcher.timer_head = pubnub_timer_list_add(m_watcher.timer_head,
                                                     pb,
                                                     pb->transaction_timeout_ms);
        pthread_mutex_unlock(&m_watcher.timerlock);
    }

    return +1;
}


void pbntf_lost_socket(pubnub_t* pb)
{
    pthread_mutex_lock(&m_watcher.mutw);
    pbpal_ntf_callback_remove_socket(m_watcher.poll, pb);
    pthread_mutex_unlock(&m_watcher.mutw);

    pbpal_ntf_callback_remove_from_queue(&m_watcher.queue, pb);

    pthread_mutex_lock(&m_watcher.timerlock);
    pbpal_remove_timer_safe(pb, &m_watcher.timer_head);
    pthread_mutex_unlock(&m_watcher.timerlock);
}


void pbntf_start_wait_connect_timer(pubnub_t* pb)
{
    if (PUBNUB_TIMERS_API) {
        pthread_mutex_lock(&m_watcher.timerlock);
        pbpal_remove_timer_safe(pb, &m_watcher.timer_head);
        m_watcher.timer_head = pubnub_timer_list_add(m_watcher.timer_head,
                                                     pb,
                                                     pb->wait_connect_timeout_ms);
        pthread_mutex_unlock(&m_watcher.timerlock);
    }
}


void pbntf_start_transaction_timer(pubnub_t* pb)
{
    if (PUBNUB_TIMERS_API) {
        pthread_mutex_lock(&m_watcher.timerlock);
        pbpal_remove_timer_safe(pb, &m_watcher.timer_head);
        m_watcher.timer_head = pubnub_timer_list_add(m_watcher.timer_head,
                                                     pb,
                                                     pb->transaction_timeout_ms);
        pthread_mutex_unlock(&m_watcher.timerlock);
    }
}


void pbntf_update_socket(pubnub_t* pb)
{
    pthread_mutex_lock(&m_watcher.mutw);
    pbpal_ntf_callback_update_socket(m_watcher.poll, pb);
    pthread_mutex_unlock(&m_watcher.mutw);
}
