/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_ntf_callback.h"

#include "pubnub_internal.h"
#include "pubnub_assert.h"
#include "pubnub_log.h"
#include "pbntf_trans_outcome_common.h"
#include "pubnub_timer_list.h"
#include "pbpal.h"
#include "pubnub_get_native_socket.h"

#include <sys/poll.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

#include <stdlib.h>
#include <string.h>

/** How many "millis" in a "nano" of some (any) unit. */
#define MILLI_IN_NANO (1000 * 1000)

/** How many "nanos" in one (unit) of something. */
#define UNIT_IN_NANO (1000 * MILLI_IN_NANO)

/** How many "millis" in one (unit) of something. */
#define UNIT_IN_MILLI (1000)

struct SocketWatcherData {
    struct pollfd *apoll pubnub_guarded_by(mutw);
    size_t apoll_size pubnub_guarded_by(mutw);
    size_t apoll_cap pubnub_guarded_by(mutw);
    pubnub_t **apb pubnub_guarded_by(mutw);
    pthread_mutex_t mutw;
    pthread_cond_t condw;
    pthread_t thread_id;
#if PUBNUB_TIMERS_API
    pubnub_t *timer_head;
#endif
    pthread_mutex_t queue_lock;
    size_t queue_size pubnub_guarded_by(queue_lock);
    size_t queue_head pubnub_guarded_by(queue_lock);
    size_t queue_tail pubnub_guarded_by(queue_lock);
    pubnub_t *queue_apb[1024] pubnub_guarded_by(queue_lock);
};

static struct SocketWatcherData m_watcher;


static void save_socket(struct SocketWatcherData *watcher, pubnub_t *pb)
{
    size_t i;
    int socket = pubnub_get_native_socket(pb);
    if (-1 == socket) {
        return;
    }
    for (i = 0; i < watcher->apoll_size; ++i) {
        PUBNUB_ASSERT_OPT(watcher->apoll[i].fd != socket);
    }
    if (watcher->apoll_size == watcher->apoll_cap) {
        size_t newcap = watcher->apoll_size + 2;
        struct pollfd *npalloc = (struct pollfd*)realloc(watcher->apoll, sizeof watcher->apoll[0] * newcap);
        pubnub_t **npapb = (pubnub_t **)realloc(watcher->apb, sizeof watcher->apb[0] * newcap);
        if (NULL == npalloc) {
            if (npapb != NULL) {
                watcher->apb = npapb;
            }
            return;
        }
        else if (NULL == npapb) {
            watcher->apoll = npalloc;
            return;
        }
        watcher->apoll = npalloc;
        watcher->apb = npapb;
        watcher->apoll_cap = newcap;
    }

    watcher->apoll[watcher->apoll_size].fd = socket;
    watcher->apoll[watcher->apoll_size].events = POLLOUT;
    watcher->apb[watcher->apoll_size] = pb;
    ++watcher->apoll_size;
}


static void remove_socket(struct SocketWatcherData *watcher, pubnub_t *pb)
{
    size_t i;
    int socket = pubnub_get_native_socket(pb);
    if (-1 == socket) {
        return;
    }
    for (i = 0; i < watcher->apoll_size; ++i) {
        if (watcher->apoll[i].fd == socket) {
            size_t to_move = watcher->apoll_size - i - 1;
            PUBNUB_ASSERT(watcher->apb[i] == pb);
            if (to_move > 0) {
                memmove(watcher->apoll + i, watcher->apoll + i + 1, sizeof watcher->apoll[0] * to_move);
                memmove(watcher->apb + i, watcher->apb + i + 1, sizeof watcher->apb[0] * to_move);
            }
            --watcher->apoll_size;
            break;
        }
    }
}


static void update_socket(struct SocketWatcherData *watcher, pubnub_t *pb)
{
    size_t i;
    int socket = pubnub_get_native_socket(pb);
    if (-1 == socket) {
        return;
    }

    for (i = 0; i < watcher->apoll_size;  ++i) {
        if (watcher->apb[i] == pb) {
            watcher->apoll[i].fd = socket;
            return;
        }
    }
}


static int elapsed_ms(struct timespec prev_timspec, struct timespec timspec)
{
    int s_diff = timspec.tv_sec - prev_timspec.tv_sec;
    int m_s_diff = (timspec.tv_nsec - prev_timspec.tv_nsec) / MILLI_IN_NANO;
    return (s_diff * UNIT_IN_MILLI) + m_s_diff;
}


int pbntf_watch_in_events(pubnub_t *pbp)
{
    unsigned i;
    for (i = 0; i < m_watcher.apoll_size; ++i) {
        if (m_watcher.apb[i] == pbp) {
            m_watcher.apoll[i].events = POLLIN;
            return 0;
        }
    }
    return -1;
}


int pbntf_watch_out_events(pubnub_t *pbp)
{
    unsigned i;
    for (i = 0; i < m_watcher.apoll_size; ++i) {
        if (m_watcher.apb[i] == pbp) {
            m_watcher.apoll[i].events = POLLOUT;
            return 0;
        }
    }
    return -1;
}

void clock_get_realtime(struct timespec *ts)
{
#if defined(__APPLE__)
	
	struct timeval tv;
	gettimeofday(&tv, NULL);
	TIMEVAL_TO_TIMESPEC(&tv, ts);
	
#else
	
	clock_gettime(CLOCK_REALTIME, ts);
	
#endif
}

void* socket_watcher_thread(void *arg)
{
    struct timespec prev_timspec;
    clock_get_realtime(&prev_timspec);

    for (;;) {
        struct timespec timspec;

        pthread_mutex_lock(&m_watcher.queue_lock);
        while (m_watcher.queue_head != m_watcher.queue_tail) {
            pubnub_t *pbp = m_watcher.queue_apb[m_watcher.queue_tail++];
            if (m_watcher.queue_tail == m_watcher.queue_size) {
                m_watcher.queue_tail = 0;
            }
            pthread_mutex_unlock(&m_watcher.queue_lock);
            if (pbp != NULL) {
                pubnub_mutex_lock(pbp->monitor);
                if (pbp->state == PBS_NULL) {
                    pubnub_mutex_unlock(pbp->monitor);
                    pballoc_free_at_last(pbp);
                }
                else {
                    pbnc_fsm(pbp);
                    pubnub_mutex_unlock(pbp->monitor);
                }
            }
            pthread_mutex_lock(&m_watcher.queue_lock);
            if (m_watcher.queue_tail == m_watcher.queue_size) {
                m_watcher.queue_tail = 0;
            }
        }
        pthread_mutex_unlock(&m_watcher.queue_lock);

        clock_get_realtime(&timspec);
        timspec.tv_sec += (timspec.tv_nsec + 200*MILLI_IN_NANO) / UNIT_IN_NANO;
        timspec.tv_nsec = (timspec.tv_nsec + 200*MILLI_IN_NANO) % UNIT_IN_NANO;

        pthread_mutex_lock(&m_watcher.mutw);
        pthread_cond_timedwait(&m_watcher.condw, &m_watcher.mutw, &timspec);

        if (m_watcher.apoll_size > 0) {
            int rslt = poll(m_watcher.apoll, m_watcher.apoll_size, 100);
            if (-1 == rslt) {
                /* error? what to do about it? */
            }
            else if (rslt > 0) {
                size_t i;
                size_t apoll_size = m_watcher.apoll_size;
                for (i = 0; i < apoll_size; ++i) {
                    if (m_watcher.apoll[i].revents & (m_watcher.apoll[i].events | POLLHUP | POLLERR | POLLNVAL)) {
                        pbntf_requeue_for_processing(m_watcher.apb[i]);
                    }
                }
            }
        }
        
        if (PUBNUB_TIMERS_API) {
            int elapsed = elapsed_ms(prev_timspec, timspec);
            if (elapsed > 0) {
                pubnub_t *expired = pubnub_timer_list_as_time_goes_by(&m_watcher.timer_head, elapsed);
                while (expired != NULL) {
                    pubnub_t *next;

                    pubnub_mutex_lock(expired->monitor);

                    next = expired->next;
                    
                    pbnc_stop(expired, PNR_TIMEOUT);
                    
                    expired->next = NULL;
                    expired->previous = NULL;
                    pubnub_mutex_unlock(expired->monitor);

                    expired = next;
                }
                prev_timspec = timspec;
            }
        }

        pthread_mutex_unlock(&m_watcher.mutw);

    }

    return NULL;
}


int pbntf_init(void)
{
    int rslt;
    pthread_mutexattr_t attr;

    rslt = pthread_mutexattr_init(&attr);
    if (rslt != 0) {
        PUBNUB_LOG_ERROR("Failed to initialize mutex attributes, error code: %d", rslt);
        return -1;
    }
    rslt = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (rslt != 0) {
        PUBNUB_LOG_ERROR("Failed to set mutex attribute type, error code: %d", rslt);
        pthread_mutexattr_destroy(&attr);
        return -1;
    }
    rslt = pthread_mutex_init(&m_watcher.mutw, &attr);
    if (rslt != 0) {
        PUBNUB_LOG_ERROR("Failed to initialize mutex, error code: %d", rslt);
        pthread_mutexattr_destroy(&attr);
        return -1;
    }

    rslt = pthread_cond_init(&m_watcher.condw, NULL);
    if (rslt != 0) {
        PUBNUB_LOG_ERROR("Failed to initialize conditional variable, error code: %d", rslt);
        pthread_mutexattr_destroy(&attr);
        pthread_mutex_destroy(&m_watcher.mutw);
        return -1;
    }

    m_watcher.queue_size = sizeof m_watcher.queue_apb / sizeof m_watcher.queue_apb[0];
    m_watcher.queue_head = m_watcher.queue_tail = 0;

    rslt = pthread_mutex_init(&m_watcher.queue_lock, &attr);
    if (rslt != 0) {
        PUBNUB_LOG_ERROR("Failed to initialize queue mutex, error code: %d", rslt);
        pthread_mutexattr_destroy(&attr);
        pthread_mutex_destroy(&m_watcher.mutw);
        return -1;
    }

#if defined(PUBNUB_CALLBACK_THREAD_STACK_SIZE_KB) && (PUBNUB_CALLBACK_THREAD_STACK_SIZE_KB > 0)
    {
        pthread_attr_t thread_attr;
        
        rslt = pthread_attr_init(&thread_attr);
        if (rslt != 0) {
            PUBNUB_LOG_ERROR("Failed to initialize thread attributes, error code: %d\n", rslt);
            pthread_mutexattr_destroy(&attr);
            pthread_mutex_destroy(&m_watcher.mutw);
            pthread_mutex_destroy(&m_watcher.queue_lock);
            return -1;
        }
        rslt = pthread_attr_setstacksize(&thread_attr, PUBNUB_CALLBACK_THREAD_STACK_SIZE_KB * 1024);
        if (rslt != 0) {
            PUBNUB_LOG_ERROR("Failed to set thread stack size to %d kb, error code: %d\n", PUBNUB_CALLBACK_THREAD_STACK_SIZE_KB, rslt);
            pthread_mutexattr_destroy(&attr);
            pthread_mutex_destroy(&m_watcher.mutw);
            pthread_mutex_destroy(&m_watcher.queue_lock);
            pthread_attr_destroy(&thread_attr);
            return -1;
        }
        rslt = pthread_create(&m_watcher.thread_id, &thread_attr, socket_watcher_thread, NULL);
        if (rslt != 0) {
            PUBNUB_LOG_ERROR("Failed to create the polling thread, error code: %d\n", rslt);
            pthread_mutexattr_destroy(&attr);
            pthread_mutex_destroy(&m_watcher.mutw);
            pthread_mutex_destroy(&m_watcher.queue_lock);
            pthread_attr_destroy(&thread_attr);
            return -1;
        }
    }
#else
    rslt = pthread_create(&m_watcher.thread_id, NULL, socket_watcher_thread, NULL);
    if (rslt != 0) {
        PUBNUB_LOG_ERROR("Failed to create the polling thread, error code: %d\n", rslt);
        pthread_mutexattr_destroy(&attr);
        pthread_mutex_destroy(&m_watcher.mutw);
        pthread_mutex_destroy(&m_watcher.queue_lock);
        return -1;
    }
#endif

    return 0;
}


int pbntf_got_socket(pubnub_t *pb, pb_socket_t socket)
{
    pthread_mutex_lock(&m_watcher.mutw);

    save_socket(&m_watcher, pb);
    if (PUBNUB_TIMERS_API) {
        m_watcher.timer_head = pubnub_timer_list_add(m_watcher.timer_head, pb);
    }
    pb->options.use_blocking_io = false;
    pbpal_set_blocking_io(pb);

    pthread_cond_signal(&m_watcher.condw);
    pthread_mutex_unlock(&m_watcher.mutw);

    return +1;
}


static void remove_timer_safe(pubnub_t *to_remove)
{
    if (PUBNUB_TIMERS_API) {
        if ((to_remove->previous != NULL) || (to_remove->next != NULL) 
            || (to_remove == m_watcher.timer_head)) {
            m_watcher.timer_head = pubnub_timer_list_remove(m_watcher.timer_head, to_remove);
        }
    }
}


static void remove_from_processing_queue(pubnub_t *pb)
{
    size_t i;

    PUBNUB_ASSERT_OPT(pb != NULL);

    pthread_mutex_lock(&m_watcher.queue_lock);
    for (i = m_watcher.queue_tail; i != m_watcher.queue_head; i = (((i + 1) == m_watcher.queue_size) ? 0 : i + 1)) {
        if (m_watcher.queue_apb[i] == pb) {
            m_watcher.queue_apb[i] = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&m_watcher.queue_lock);
}


void pbntf_lost_socket(pubnub_t *pb, pb_socket_t socket)
{
    PUBNUB_UNUSED(socket);

    pthread_mutex_lock(&m_watcher.mutw);

    remove_socket(&m_watcher, pb);
    remove_timer_safe(pb);
    remove_from_processing_queue(pb);

    pthread_cond_signal(&m_watcher.condw);
    pthread_mutex_unlock(&m_watcher.mutw);
}


void pbntf_update_socket(pubnub_t *pb, pb_socket_t socket)
{
    PUBNUB_UNUSED(socket);

    pthread_mutex_lock(&m_watcher.mutw);

    update_socket(&m_watcher, pb);

    pthread_cond_signal(&m_watcher.condw);
    pthread_mutex_unlock(&m_watcher.mutw);
}


void pbntf_trans_outcome(pubnub_t *pb)
{
    PBNTF_TRANS_OUTCOME_COMMON(pb);
    if (pb->cb != NULL) {
        pb->cb(pb, pb->trans, pb->core.last_result, pb->user_data);
    }
}



int pbntf_enqueue_for_processing(pubnub_t *pb)
{
    int result;
    size_t next_head;

    PUBNUB_ASSERT_OPT(pb != NULL);

    pthread_mutex_lock(&m_watcher.queue_lock);
    next_head = m_watcher.queue_head + 1;
    if (next_head == m_watcher.queue_size) {
        next_head = 0;
    }
    if (next_head != m_watcher.queue_tail) {
        m_watcher.queue_apb[m_watcher.queue_head] = pb;
        m_watcher.queue_head = next_head;
        result = +1;
    }
    else {
        result = -1;
    }
    pthread_mutex_unlock(&m_watcher.queue_lock);

    return result;
}


int pbntf_requeue_for_processing(pubnub_t *pb)
{
    bool found = false;
    size_t i;

    PUBNUB_ASSERT_OPT(pb != NULL);

    pthread_mutex_lock(&m_watcher.queue_lock);
    for (i = m_watcher.queue_tail; i != m_watcher.queue_head; i = (((i + 1) == m_watcher.queue_size) ? 0 : i + 1)) {
        if (m_watcher.queue_apb[i] == pb) {
            found = true;
            break;
        }
    }
    pthread_mutex_unlock(&m_watcher.queue_lock);

    return !found ? pbntf_enqueue_for_processing(pb) : 0;
}


enum pubnub_res pubnub_last_result(pubnub_t *pb)
{
    enum pubnub_res rslt;
    
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    
    pubnub_mutex_lock(pb->monitor);
    rslt = pb->core.last_result;
    pubnub_mutex_unlock(pb->monitor);
    
    return rslt;
}


enum pubnub_res pubnub_register_callback(pubnub_t *pb, pubnub_callback_t cb, void *user_data)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    pubnub_mutex_lock(pb->monitor);
    pb->cb = cb;
    pb->user_data = user_data;
    pubnub_mutex_unlock(pb->monitor);
    return PNR_OK;
}


void *pubnub_get_user_data(pubnub_t *pb)
{
    void *result;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    result = pb->user_data;
    pubnub_mutex_unlock(pb->monitor);

    return result;
}


pubnub_callback_t pubnub_get_callback(pubnub_t *pb)
{
    pubnub_callback_t result;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    result = pb->cb;
    pubnub_mutex_unlock(pb->monitor);

    return result;
}
