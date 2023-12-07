/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if PUBNUB_USE_AUTO_HEARTBEAT

#include "pubnub_internal.h"
#include "core/pubnub_log.h"

#include <pthread.h>

static int create_heartbeat_watcher_thread(struct HeartbeatWatcherData* m_watcher)
{
    int rslt;

#if defined(PUBNUB_CALLBACK_THREAD_STACK_SIZE_KB)                              \
    && (PUBNUB_CALLBACK_THREAD_STACK_SIZE_KB > 0)
    {
        pthread_attr_t thread_attr;

        rslt = pthread_attr_init(&thread_attr);
        if (rslt != 0) {
            PUBNUB_LOG_ERROR(
                "create_heartbeat_watcher_thread() - "
                "Failed to initialize thread attributes, error code: %d\n",
                rslt);
            return -1;
        }
        rslt = pthread_attr_setstacksize(
            &thread_attr, PUBNUB_CALLBACK_THREAD_STACK_SIZE_KB * 1024);
        if (rslt != 0) {
            PUBNUB_LOG_ERROR(
                "create_heartbeat_watcher_thread() - "
                "Failed to set thread stack size to %d kb, error code: %d\n",
                PUBNUB_CALLBACK_THREAD_STACK_SIZE_KB,
                rslt);
            pthread_attr_destroy(&thread_attr);
            return -1;
        }
        rslt = pthread_create(
            &m_watcher->thread_id, &thread_attr, pbauto_heartbeat_watcher_thread, NULL);
        if (rslt != 0) {
            PUBNUB_LOG_ERROR("create_heartbeat_watcher_thread() - "
                             "Failed to create the auto heartbeat watcher "
                             "thread, error code: %d\n",
                             rslt);
            pthread_attr_destroy(&thread_attr);
            return -1;
        }
    }
#else
    rslt = pthread_create(&m_watcher->thread_id, NULL, pbauto_heartbeat_watcher_thread,
NULL); if (rslt != 0) { PUBNUB_LOG_ERROR("create_heartbeat_watcher_thread() - "
                         "Failed to create the auto heartbeat watcher thread, "
                         "error code: %d\n",
                         rslt);
        return -1;
    }
#endif

    return 0;
}


int pbauto_heartbeat_init(struct HeartbeatWatcherData* m_watcher)
{
    int                 rslt;
    pthread_mutexattr_t attr;

    rslt = pthread_mutexattr_init(&attr);
    if (rslt != 0) {
        PUBNUB_LOG_ERROR("pbauto_heartbeat_init() - Failed to initialize mutex "
                         "attributes, error code: %d\n",
                         rslt);
        return -1;
    }
    rslt = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (rslt != 0) {
        PUBNUB_LOG_ERROR("pbauto_heartbeat_init() - Failed to set mutex "
                         "attribute type, error code: %d\n",
                         rslt);
        pthread_mutexattr_destroy(&attr);
        return -1;
    }
    rslt = pthread_mutex_init(&m_watcher->stoplock, &attr);
    if (rslt != 0) {
        PUBNUB_LOG_ERROR("pbauto_heartbeat_init() - Failed to initialize "
                         "'stoplock' mutex, error code: %d\n",
                         rslt);
        pthread_mutexattr_destroy(&attr);
        return -1;
    }
    rslt = pthread_mutex_init(&m_watcher->mutw, &attr);
    if (rslt != 0) {
        PUBNUB_LOG_ERROR("pbauto_heartbeat_init() - Failed to initialize mutex, "
                         "error code: %d\n",
                         rslt);
        pthread_mutexattr_destroy(&attr);
        pubnub_mutex_destroy(m_watcher->stoplock);
        return -1;
    }
    rslt = pthread_mutex_init(&m_watcher->timerlock, &attr);
    if (rslt != 0) {
        PUBNUB_LOG_ERROR("pbauto_heartbeat_init() - Failed to initialize mutex "
                         "for heartbeat timers, "
                         "error code: %d\n",
                         rslt);
        pthread_mutexattr_destroy(&attr);
        pubnub_mutex_destroy(m_watcher->mutw);
        pubnub_mutex_destroy(m_watcher->stoplock);
        return -1;
    }
    m_watcher->stop_heartbeat_watcher_thread = false;
    rslt = create_heartbeat_watcher_thread(m_watcher);
    if (rslt != 0) {
        PUBNUB_LOG_ERROR(
            "pbauto_heartbeat_init() - create_heartbeat_watcher_thread() failed, "
            "error code: %d\n",
            rslt);
        pthread_mutexattr_destroy(&attr);
        pubnub_mutex_destroy(m_watcher->mutw);
        pubnub_mutex_destroy(m_watcher->timerlock);
        pubnub_mutex_destroy(m_watcher->stoplock);
        return -1;
    }

    return 0;
}

#endif
