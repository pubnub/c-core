/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "core/pubnub_ntf_callback.h"

#include <winsock2.h>
#include <windows.h>
#include <process.h>

#include "pubnub_internal.h"
#include "core/pubnub_assert.h"
#include "core/pubnub_log.h"
#include "core/pubnub_timer_list.h"
#include "core/pbpal.h"

#include "lib/sockets/pbpal_ntf_callback_poller_poll.h"
#include "core/pbpal_ntf_callback_queue.h"
#include "core/pbpal_ntf_callback_handle_timer_list.h"

#include <stdlib.h>
#include <string.h>


#if !defined _Guarded_by_
#define _Guarded_by_(x)
#endif


/** The number of Windows FILETIME intervals in a millisecond. Windows
    FILETIME interval is 100 ns.  In practice, the actual resolution
    may be (much) different, but, nominally it's 100ns.
*/
#define MSEC_IN_FILETIME_INTERVALS (10 * 1000)


struct SocketWatcherData {
    _Guarded_by_(mutw) struct pbpal_poll_data* poll;
    _Guarded_by_(stoplock) bool stop_socket_watcher_thread;
    CRITICAL_SECTION mutw;
    CRITICAL_SECTION timerlock;
    CRITICAL_SECTION stoplock;
    HANDLE           thread_handle;
    DWORD            thread_id;
#if PUBNUB_TIMERS_API
    _Guarded_by_(timerlock) pubnub_t* timer_head;
#endif
    struct pbpal_ntf_callback_queue queue;
};


static struct SocketWatcherData m_watcher;


static int elapsed_ms(FILETIME prev_timspec, FILETIME timspec)
{
    ULARGE_INTEGER prev;
    ULARGE_INTEGER current;
    prev.LowPart     = prev_timspec.dwLowDateTime;
    prev.HighPart    = prev_timspec.dwHighDateTime;
    current.LowPart  = timspec.dwLowDateTime;
    current.HighPart = timspec.dwHighDateTime;
    return (int)((current.QuadPart - prev.QuadPart) / MSEC_IN_FILETIME_INTERVALS);
}


int pbntf_watch_in_events(pubnub_t* pbp)
{
    return pbpal_ntf_watch_in_events(m_watcher.poll, pbp);
}


int pbntf_watch_out_events(pubnub_t* pbp)
{
    return pbpal_ntf_watch_out_events(m_watcher.poll, pbp);
}


void socket_watcher_thread(void* arg)
{
    FILETIME prev_time;
    GetSystemTimeAsFileTime(&prev_time);

    PUBNUB_UNUSED(arg);

    for (;;) {
        const DWORD ms = 100;
        bool        stop_thread;

        EnterCriticalSection(&m_watcher.stoplock);
        stop_thread = m_watcher.stop_socket_watcher_thread;
        LeaveCriticalSection(&m_watcher.stoplock);
        if (stop_thread) {
            break;
        }

        pbpal_ntf_callback_process_queue(&m_watcher.queue);

        EnterCriticalSection(&m_watcher.mutw);
        pbpal_ntf_poll_away(m_watcher.poll, ms);
        LeaveCriticalSection(&m_watcher.mutw);

        if (PUBNUB_TIMERS_API) {
            FILETIME current_time;
            int      elapsed;
            GetSystemTimeAsFileTime(&current_time);
            elapsed = elapsed_ms(prev_time, current_time);
            if (elapsed > 0) {
                EnterCriticalSection(&m_watcher.timerlock);
                pbntf_handle_timer_list(elapsed, &m_watcher.timer_head);
                LeaveCriticalSection(&m_watcher.timerlock);

                prev_time = current_time;
            }
        }
    }
}


int pbntf_init(void)
{
    InitializeCriticalSection(&m_watcher.stoplock);
    InitializeCriticalSection(&m_watcher.mutw);
    InitializeCriticalSection(&m_watcher.timerlock);

    m_watcher.stop_socket_watcher_thread = false;
    m_watcher.poll = pbpal_ntf_callback_poller_init();
    if (NULL == m_watcher.poll) {
        return -1;
    }
    pbpal_ntf_callback_queue_init(&m_watcher.queue);

    m_watcher.thread_handle = (HANDLE)_beginthread(
        socket_watcher_thread, PUBNUB_CALLBACK_THREAD_STACK_SIZE_KB * 1024, NULL);
    if ((HANDLE)-1L == m_watcher.thread_handle) {
        PUBNUB_LOG_ERROR("Failed to start the polling thread, error code: %d\n",
                         errno);
        DeleteCriticalSection(&m_watcher.mutw);
        DeleteCriticalSection(&m_watcher.timerlock);
        DeleteCriticalSection(&m_watcher.stoplock);
        pbpal_ntf_callback_queue_deinit(&m_watcher.queue);
        pbpal_ntf_callback_poller_deinit(&m_watcher.poll);
        return -1;
    }
    m_watcher.thread_id = GetThreadId(m_watcher.thread_handle);

    return 0;
}


void pubnub_stop(void)
{
    EnterCriticalSection(&m_watcher.stoplock);
    m_watcher.stop_socket_watcher_thread = true;
    LeaveCriticalSection(&m_watcher.stoplock);
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
    EnterCriticalSection(&m_watcher.mutw);
    pbpal_ntf_callback_save_socket(m_watcher.poll, pb);
    LeaveCriticalSection(&m_watcher.mutw);

    if (PUBNUB_TIMERS_API) {
        EnterCriticalSection(&m_watcher.timerlock);
        m_watcher.timer_head = pubnub_timer_list_add(m_watcher.timer_head,
                                                     pb,
                                                     pb->transaction_timeout_ms);
        LeaveCriticalSection(&m_watcher.timerlock);
    }

    return +1;
}


void pbntf_lost_socket(pubnub_t* pb)
{
    EnterCriticalSection(&m_watcher.mutw);
    pbpal_ntf_callback_remove_socket(m_watcher.poll, pb);
    LeaveCriticalSection(&m_watcher.mutw);

    pbpal_ntf_callback_remove_from_queue(&m_watcher.queue, pb);

    EnterCriticalSection(&m_watcher.timerlock);
    pbpal_remove_timer_safe(pb, &m_watcher.timer_head);
    LeaveCriticalSection(&m_watcher.timerlock);
}


void pbntf_start_wait_connect_timer(pubnub_t* pb)
{
    if (PUBNUB_TIMERS_API) {
        EnterCriticalSection(&m_watcher.timerlock);
        pbpal_remove_timer_safe(pb, &m_watcher.timer_head);
        m_watcher.timer_head = pubnub_timer_list_add(m_watcher.timer_head,
                                                     pb,
                                                     pb->wait_connect_timeout_ms);
        LeaveCriticalSection(&m_watcher.timerlock);
    }
}


void pbntf_start_transaction_timer(pubnub_t* pb)
{
    if (PUBNUB_TIMERS_API) {
        EnterCriticalSection(&m_watcher.timerlock);
        pbpal_remove_timer_safe(pb, &m_watcher.timer_head);
        m_watcher.timer_head = pubnub_timer_list_add(m_watcher.timer_head,
                                                     pb,
                                                     pb->transaction_timeout_ms);
        LeaveCriticalSection(&m_watcher.timerlock);
    }
}


void pbntf_update_socket(pubnub_t* pb)
{
    EnterCriticalSection(&m_watcher.mutw);
    pbpal_ntf_callback_update_socket(m_watcher.poll, pb);
    LeaveCriticalSection(&m_watcher.mutw);
}
