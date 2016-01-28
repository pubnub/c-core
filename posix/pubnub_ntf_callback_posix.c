/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_ntf_callback.h"

#include "../posix/monotonic_clock_get_time.h"

#include "pubnub_internal.h"
#include "pubnub_assert.h"
#include "pubnub_log.h"
#include "pbntf_trans_outcome_common.h"
#include "pubnub_timer_list.h"
#include "pbpal.h"
#include "pubnub_get_native_socket.h"

#include <sys/poll.h>
#include <pthread.h>

#include <stdlib.h>
#include <string.h>


struct SocketWatcherData {
    struct pollfd *apoll;
    size_t apoll_size;
    size_t apoll_cap;
    pubnub_t **apb;
    pthread_mutex_t mutw;
    pthread_cond_t condw;
    pthread_t thread_id;
#if PUBNUB_TIMERS_API
    pubnub_t *timer_head;
#endif
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


static int elapsed_ms(struct timespec prev_timspec, struct timespec timspec)
{
    int s_diff = timspec.tv_sec - prev_timspec.tv_sec;
    int m_s_diff = (timspec.tv_nsec - prev_timspec.tv_nsec) / MILLI_IN_NANO;
    return (s_diff * UNIT_IN_MILLI) + m_s_diff;
}


void* socket_watcher_thread(void *arg)
{
    struct timespec prev_timspec;
    monotonic_clock_get_time(&prev_timspec);

    for (;;) {
        struct timespec timspec;

        monotonic_clock_get_time(&timspec);
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
                for (i = 0; i < m_watcher.apoll_size; ++i) {
                    if (m_watcher.apoll[i].revents & (m_watcher.apoll[i].events | POLLHUP)) {
                        pubnub_mutex_lock(m_watcher.apb[i]->monitor);
                        pbnc_fsm(m_watcher.apb[i]);
                        if ((m_watcher.apoll[i].events == POLLOUT) && 
                            (m_watcher.apb[i]->state >= PBS_RX_HTTP_VER)) {
                            m_watcher.apoll[i].events = POLLIN;
                        }
                        pubnub_mutex_unlock(m_watcher.apb[i]->monitor);
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
    pthread_mutexattr_t attr;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&m_watcher.mutw, &attr);

    pthread_cond_init(&m_watcher.condw, NULL);

    pthread_create(&m_watcher.thread_id, NULL, socket_watcher_thread, NULL);

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


void pbntf_lost_socket(pubnub_t *pb, pb_socket_t socket)
{
    pthread_mutex_lock(&m_watcher.mutw);

    remove_socket(&m_watcher, pb);
    remove_timer_safe(pb);

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


enum pubnub_res pubnub_last_result(pubnub_t *pb)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    return pb->core.last_result;
}


enum pubnub_res pubnub_register_callback(pubnub_t *pb, pubnub_callback_t cb, void *user_data)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    pb->cb = cb;
    pb->user_data = user_data;
    return PNR_OK;
}


void *pubnub_get_user_data(pubnub_t *pb)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    return pb->user_data;
}
