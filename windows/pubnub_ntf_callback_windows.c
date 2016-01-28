/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_ntf_callback.h"

#include <winsock2.h>
#include <windows.h>
#include <process.h>

#include "pubnub_internal.h"
#include "pubnub_assert.h"
#include "pubnub_log.h"
#include "pbntf_trans_outcome_common.h"
#include "pubnub_timer_list.h"
#include "pbpal.h"


#include "pubnub_get_native_socket.h"

#include <stdlib.h>
#include <string.h>


struct SocketWatcherData {
    _Guarded_by_(mutw) size_t apoll_size;
    _Guarded_by_(mutw) size_t apoll_cap;
    _Guarded_by_(mutw) _Field_size_(apoll_cap) WSAPOLLFD *apoll;
    _Guarded_by_(mutw) _Field_size_(apoll_cap) pubnub_t **apb;
    CRITICAL_SECTION mutw;
    HANDLE condw;
    uintptr_t thread_id;
#if PUBNUB_TIMERS_API
    _Guarded_by_(mutw) pubnub_t *timer_head;
#endif
};

static struct SocketWatcherData m_watcher;


static void save_socket(struct SocketWatcherData *watcher, pubnub_t *pb)
{
    size_t i;
    SOCKET sockt = pubnub_get_native_socket(pb);

    PUBNUB_ASSERT(watcher->apoll_size <= watcher->apoll_cap);

    if (INVALID_SOCKET == sockt) {
        return ;
    }
    for (i = 0; i < watcher->apoll_size; ++i) {
        PUBNUB_ASSERT_OPT(watcher->apoll[i].fd != sockt);
    }
    if (watcher->apoll_size == watcher->apoll_cap) {
        size_t newcap = watcher->apoll_cap + 2;
        WSAPOLLFD *npapoll = (WSAPOLLFD*)realloc(watcher->apoll, sizeof watcher->apoll[0] * newcap);
        if (NULL == npapoll) {
            return;
        }
        else {
            pubnub_t **npapb = (pubnub_t**)realloc(watcher->apb, sizeof watcher->apb[0] * newcap);
            watcher->apoll = npapoll;
            if (NULL == npapb) {
                return;
            }
            watcher->apb = npapb;
        }
        watcher->apoll_cap = newcap;
    }

    watcher->apoll[watcher->apoll_size].fd = sockt;
    watcher->apoll[watcher->apoll_size].events = POLLIN | POLLOUT;
    watcher->apb[watcher->apoll_size] = pb;
    ++watcher->apoll_size;
}


static void remove_socket(struct SocketWatcherData *watcher, pubnub_t *pb)
{
    size_t i;
    SOCKET sockt = pubnub_get_native_socket(pb);

    PUBNUB_ASSERT(watcher->apoll_size <= watcher->apoll_cap);

    if (INVALID_SOCKET == sockt) {
        return ;
    }
    for (i = 0; i < watcher->apoll_size; ++i) {
        if (watcher->apoll[i].fd == sockt) {
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


static int elapsed_ms(FILETIME prev_timspec, FILETIME timspec)
{
    ULARGE_INTEGER prev;
    ULARGE_INTEGER current;
    prev.LowPart = prev_timspec.dwLowDateTime;
    prev.HighPart = prev_timspec.dwHighDateTime;
    current.LowPart = timspec.dwLowDateTime;
    current.HighPart = timspec.dwHighDateTime;
    return (int)((current.QuadPart - prev.QuadPart) / (10 * 1000));
}


void socket_watcher_thread(void *arg)
{
    FILETIME prev_time;
    GetSystemTimeAsFileTime(&prev_time);

    for (;;) {
        const DWORD ms = 100;

        WaitForSingleObject(m_watcher.condw, ms);

        EnterCriticalSection(&m_watcher.mutw);
        if (0 == m_watcher.apoll_size) {
            LeaveCriticalSection(&m_watcher.mutw);
            continue;
        }
        {
            int rslt = WSAPoll(m_watcher.apoll, m_watcher.apoll_size, ms);
            if (SOCKET_ERROR == rslt) {
                /* error? what to do about it? */
                PUBNUB_LOG_WARNING("poll size = %d, error = %d\n", m_watcher.apoll_size, WSAGetLastError());
            }
            else if (rslt > 0) {
                size_t i;
                for (i = 0; i < m_watcher.apoll_size; ++i) {
                    if (m_watcher.apoll[i].revents & (POLLIN | POLLOUT)) {
                        pubnub_mutex_lock(m_watcher.apb[i]->monitor);
                        pbnc_fsm(m_watcher.apb[i]);
                        pubnub_mutex_unlock(m_watcher.apb[i]->monitor);
                    }
                }
            }
        }
        if (PUBNUB_TIMERS_API) {
            FILETIME current_time;
            int elapsed;
            GetSystemTimeAsFileTime(&current_time);
            elapsed = elapsed_ms(prev_time, current_time);
            if (elapsed > 0) {
                pubnub_t *expired = pubnub_timer_list_as_time_goes_by(&m_watcher.timer_head, elapsed);
                while (expired != NULL) {
                    pubnub_t *next = expired->next;

                    pubnub_mutex_lock(expired->monitor);
                    pbnc_stop(expired, PNR_TIMEOUT);
                    pubnub_mutex_unlock(expired->monitor);

                    expired->next = NULL;
                    expired->previous = NULL;
                    expired = next;
                }
                prev_time = current_time;
            }
        }

        LeaveCriticalSection(&m_watcher.mutw);
    }
}


int pbntf_init(void)
{
    InitializeCriticalSection(&m_watcher.mutw);
    m_watcher.thread_id = _beginthread(socket_watcher_thread, 0, NULL);

    return 0;
}


int pbntf_got_socket(pubnub_t *pb, pb_socket_t socket)
{
    EnterCriticalSection(&m_watcher.mutw);

    save_socket(&m_watcher, pb);
    if (PUBNUB_TIMERS_API) {
        m_watcher.timer_head = pubnub_timer_list_add(m_watcher.timer_head, pb);
    }
    pb->options.use_blocking_io = false;
    pbpal_set_blocking_io(pb);
    
    LeaveCriticalSection(&m_watcher.mutw);

    SetEvent(m_watcher.condw);

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
    EnterCriticalSection(&m_watcher.mutw);

    remove_socket(&m_watcher, pb);
    remove_timer_safe(pb);

    LeaveCriticalSection(&m_watcher.mutw);
    SetEvent(m_watcher.condw);
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
