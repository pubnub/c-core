/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if PUBNUB_USE_AUTO_HEARTBEAT

#include "pubnub_internal.h"
#include "core/pubnub_log.h"

#include <process.h>

#if !defined PUBNUB_CALLBACK_THREAD_STACK_SIZE_KB
#define PUBNUB_CALLBACK_THREAD_STACK_SIZE_KB 0
#endif

int pbauto_heartbeat_init(struct HeartbeatWatcherData* m_watcher)
{
    InitializeCriticalSection(&m_watcher->stoplock);
    InitializeCriticalSection(&m_watcher->mutw);
    InitializeCriticalSection(&m_watcher->timerlock);

    m_watcher->stop_heartbeat_watcher_thread = false;

    m_watcher->thread_handle = (HANDLE)_beginthread(
        pbauto_heartbeat_watcher_thread, PUBNUB_CALLBACK_THREAD_STACK_SIZE_KB * 1024, NULL);
    if ((HANDLE)-1L == m_watcher->thread_handle) {
        PUBNUB_LOG_ERROR("Failed to start the polling thread, error code: %d\n",
                         errno);
        DeleteCriticalSection(&m_watcher->mutw);
        DeleteCriticalSection(&m_watcher->timerlock);
        DeleteCriticalSection(&m_watcher->stoplock);
        return -1;
    }
    m_watcher->thread_id = GetThreadId(m_watcher->thread_handle);

    return 0;
}

#endif /* PUBNUB_USE_AUTO_HEARTBEAT */

