/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_ntf_callback.h"

#include "pubnub_internal.h"
#include "pubnub_assert.h"
#include "pbntf_trans_outcome_common.h"
#include "pbpal.h"

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
};

static struct SocketWatcherData m_watcher;


static void save_socket(struct SocketWatcherData *watcher, pubnub_t *pb, pb_socket_t socket)
{
    if (watcher->apoll_size == watcher->apoll_cap) {
        struct pollfd *npalloc = (struct pollfd*)realloc(watcher->apoll, sizeof watcher->apoll[0] * (watcher->apoll_size+1));
        pubnub_t **npapb = (pubnub_t **)realloc(watcher->apb, sizeof watcher->apb[0] * (watcher->apoll_size+1));
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
        watcher->apoll[watcher->apoll_size].fd = socket;
        watcher->apoll[watcher->apoll_size].events = POLLIN | POLLOUT;
        watcher->apb[watcher->apoll_size] = pb;
        watcher->apoll_cap = ++watcher->apoll_size;
    }
    else {
        watcher->apoll[watcher->apoll_size].fd = socket;
        watcher->apoll[watcher->apoll_size].events = POLLIN | POLLOUT;
        watcher->apb[watcher->apoll_size] = pb;
        ++watcher->apoll_size;
    }
}


static void remove_socket(struct SocketWatcherData *watcher, pubnub_t *pb, pb_socket_t socket)
{
    size_t i;
    for (i = 0; i < watcher->apoll_size; ++i) {
        if (watcher->apoll[i].fd == socket) {
            PUBNUB_ASSERT(watcher->apb[i] == pb);
            if (i != watcher->apoll_size - 1) {
                memmove(watcher->apoll + i, watcher->apoll + i + 1, sizeof watcher->apoll[0] * (watcher->apoll_size - i - 1));
                memmove(watcher->apb + i, watcher->apb + i + 1, sizeof watcher->apb[0] * (watcher->apoll_size - i - 1));
            }
            --watcher->apoll_size;
            break;
        }
    }
}


void* socket_watcher_thread(void *arg)
{
    for (;;) {
        struct timespec timspec;

        clock_gettime(CLOCK_MONOTONIC, &timspec);
        timspec.tv_sec = (timspec.tv_nsec + 300) / 1000;
        timspec.tv_nsec = (timspec.tv_nsec + 300) % 1000;
        
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
                    if (m_watcher.apoll[i].revents & (POLLIN | POLLOUT | POLLHUP)) {
                        pbnc_fsm(m_watcher.apb[i]);
                    }
                }
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

    save_socket(&m_watcher, pb, socket);
    pb->use_blocking_io = false;
    pbpal_set_blocking_io(pb);

    pthread_cond_signal(&m_watcher.condw);
    pthread_mutex_unlock(&m_watcher.mutw);

    return +1;
}


void pbntf_lost_socket(pubnub_t *pb, pb_socket_t socket)
{
    pthread_mutex_lock(&m_watcher.mutw);

    remove_socket(&m_watcher, pb, socket);

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


enum pubnub_res pubnub_last_result(pubnub_t const *pb)
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
