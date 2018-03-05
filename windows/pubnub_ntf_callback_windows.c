/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_ntf_callback.h"

#include <winsock2.h>
#include <windows.h>
#include <process.h>

#include "pubnub_internal.h"
#include "pubnub_assert.h"
#include "pubnub_log.h"
#include "pubnub_timer_list.h"
#include "pbpal.h"

#include "pbpal_ntf_callback_poller_poll.h"
#include "pbpal_ntf_callback_queue.h"
#include "pbpal_ntf_callback_handle_timer_list.h"

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
    CRITICAL_SECTION mutw;
    HANDLE           thread_handle;
    DWORD            thread_id;
#if PUBNUB_TIMERS_API
    _Guarded_by_(mutw) pubnub_t* timer_head;
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

        pbpal_ntf_callback_process_queue(&m_watcher.queue);

        Sleep(1);

        EnterCriticalSection(&m_watcher.mutw);
        pbpal_ntf_poll_away(m_watcher.poll, ms);
        LeaveCriticalSection(&m_watcher.mutw);

        if (PUBNUB_TIMERS_API) {
            FILETIME current_time;
            int      elapsed;
            GetSystemTimeAsFileTime(&current_time);
            elapsed = elapsed_ms(prev_time, current_time);
            if (elapsed > 0) {
                EnterCriticalSection(&m_watcher.mutw);
                pbntf_handle_timer_list(elapsed, &m_watcher.timer_head);
                LeaveCriticalSection(&m_watcher.mutw);

                prev_time = current_time;
            }
        }
    }
}


int pbntf_init(void)
{
    InitializeCriticalSection(&m_watcher.mutw);

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
        pbpal_ntf_callback_queue_deinit(&m_watcher.queue);
        pbpal_ntf_callback_poller_deinit(&m_watcher.poll);
        return -1;
    }
    m_watcher.thread_id = GetThreadId(m_watcher.thread_handle);

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


int pbntf_got_socket(pubnub_t* pb, pb_socket_t sockt)
{
    EnterCriticalSection(&m_watcher.mutw);

    PUBNUB_UNUSED(sockt);

    pbpal_ntf_callback_save_socket(m_watcher.poll, pb);
    if (PUBNUB_TIMERS_API) {
        m_watcher.timer_head = pubnub_timer_list_add(m_watcher.timer_head, pb);
    }
    pb->options.use_blocking_io = false;
    pbpal_set_blocking_io(pb);

    LeaveCriticalSection(&m_watcher.mutw);

    return +1;
}


void pbntf_lost_socket(pubnub_t* pb, pb_socket_t sockt)
{
    EnterCriticalSection(&m_watcher.mutw);

    PUBNUB_UNUSED(sockt);

    pbpal_ntf_callback_remove_socket(m_watcher.poll, pb);
    pbpal_remove_timer_safe(pb, &m_watcher.timer_head);
    pbpal_ntf_callback_remove_from_queue(&m_watcher.queue, pb);

    LeaveCriticalSection(&m_watcher.mutw);
}


void pbntf_update_socket(pubnub_t* pb, pb_socket_t socket)
{
    PUBNUB_UNUSED(socket);

    EnterCriticalSection(&m_watcher.mutw);

    pbpal_ntf_callback_update_socket(m_watcher.poll, pb);

    LeaveCriticalSection(&m_watcher.mutw);
}
