/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if PUBNUB_USE_AUTO_HEARTBEAT

#include "pubnub_internal.h"

#include <pthread.h>

static int create_heartbeat_watcher_thread(
    pubnub_t*                    pb,
    struct HeartbeatWatcherData* m_watcher)
{
    int rslt;

#if defined(PUBNUB_CALLBACK_THREAD_STACK_SIZE_KB) &&                           \
    (PUBNUB_CALLBACK_THREAD_STACK_SIZE_KB > 0)
    {
        pthread_attr_t thread_attr;

        rslt = pthread_attr_init(&thread_attr);
        if (rslt != 0) {
            PUBNUB_LOG_ERROR(
                pb,
                "Thread attributes initialization failed with error code: %d",
                rslt);
            return -1;
        }
        rslt = pthread_attr_setstacksize(
            &thread_attr, PUBNUB_CALLBACK_THREAD_STACK_SIZE_KB * 1024);
        if (rslt != 0) {
            PUBNUB_LOG_ERROR(
                pb,
                "Thread stack size change to %d kb failed with error code: %d",
                PUBNUB_CALLBACK_THREAD_STACK_SIZE_KB,
                error);
            pthread_attr_destroy(&thread_attr);
            return -1;
        }
        rslt = pthread_create(
            &m_watcher->thread_id,
            &thread_attr,
            pbauto_heartbeat_watcher_thread,
            NULL);
        if (rslt != 0) {
            PUBNUB_LOG_ERROR(
                pb,
                "Auto heartbeat watcher thread create failed with error code: "
                "%d",
                rslt);
            pthread_attr_destroy(&thread_attr);
            return -1;
        }
    }
#else
    rslt = pthread_create(
        &m_watcher->thread_id, NULL, pbauto_heartbeat_watcher_thread, NULL);
    if (rslt != 0) {
        PUBNUB_LOG_ERROR(
            pb,
            "Auto heartbeat watcher thread create failed with error code: %d",
            rslt);
        return -1;
    }
#endif

    return 0;
}


int pbauto_heartbeat_init(pubnub_t* pb, struct HeartbeatWatcherData* m_watcher)
{
    int                 rslt;
    pthread_mutexattr_t attr;

    rslt = pthread_mutexattr_init(&attr);
    if (rslt != 0) {
        PUBNUB_LOG_ERROR(
            pb,
            "Mutex attributes initialization failed with error code: %d",
            rslt);
        return -1;
    }
    rslt = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (rslt != 0) {
        PUBNUB_LOG_ERROR(
            pb, "Mutex attribute type set failed with error code: %d", rslt);
        pthread_mutexattr_destroy(&attr);
        return -1;
    }
    rslt = pthread_mutex_init(&m_watcher->stoplock, &attr);
    if (rslt != 0) {
        PUBNUB_LOG_ERROR(
            pb,
            "'stoplock' mutex initialization failed with error code: %d",
            rslt);
        pthread_mutexattr_destroy(&attr);
        return -1;
    }
    rslt = pthread_mutex_init(&m_watcher->mutw, &attr);
    if (rslt != 0) {
        PUBNUB_LOG_ERROR(
            pb, "Mutex initialization failed with error code: %d", rslt);
        pthread_mutexattr_destroy(&attr);
        pubnub_mutex_destroy(m_watcher->stoplock);
        return -1;
    }
    rslt = pthread_mutex_init(&m_watcher->timerlock, &attr);
    if (rslt != 0) {
        PUBNUB_LOG_ERROR(
            pb,
            "Timer's mutex initialization failed with error code: %d",
            rslt);
        pthread_mutexattr_destroy(&attr);
        pubnub_mutex_destroy(m_watcher->mutw);
        pubnub_mutex_destroy(m_watcher->stoplock);
        return -1;
    }
    m_watcher->stop_heartbeat_watcher_thread = false;
    rslt = create_heartbeat_watcher_thread(pb, m_watcher);
    if (rslt != 0) {
        PUBNUB_LOG_ERROR(
            pb, "Polling thread create failed with error code: %d", rslt);
        pthread_mutexattr_destroy(&attr);
        pubnub_mutex_destroy(m_watcher->mutw);
        pubnub_mutex_destroy(m_watcher->timerlock);
        pubnub_mutex_destroy(m_watcher->stoplock);
        return -1;
    }

    return 0;
}

#endif
