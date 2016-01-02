/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_ntf_callback.h"

#include "pubnub_internal.h"
#include "pubnub_assert.h"
#include "pbntf_trans_outcome_common.h"

#include "pbpal.h"

#include <windows.h>
#include <process.h>

#include <stdlib.h>
#include <string.h>


struct SocketWatcherData {
    size_t apoll_size;
    size_t apoll_cap;
    WSAPOLLFD *apoll;
    pubnub_t **apb;
#if PUBNUB_TIMERS_API
    /*_Field_size_(apoll_cap) */HANDLE *atimer;
#endif
    CRITICAL_SECTION mutw;
    HANDLE condw;
    uintptr_t thread_id;
};

static struct SocketWatcherData m_watcher;


static void save_socket(struct SocketWatcherData *watcher, pubnub_t *pb, HANDLE timer)
{
    PUBNUB_ASSERT_OPT(watcher->apoll_size < MAXIMUM_WAIT_OBJECTS);
    PUBNUB_ASSERT(watcher->apoll_size <= watcher->apoll_cap);
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
            else {
                HANDLE *npatmr = (HANDLE*)realloc(watcher->atimer, sizeof watcher->atimer[0] * newcap);
                watcher->apb = npapb;
                if (NULL == npatmr) {
                    return;
                }
                watcher->atimer = npatmr;
            }
        }
        watcher->apoll_cap = newcap;
    }

    watcher->apoll[watcher->apoll_size].fd = pb->pal.socket;
    watcher->apoll[watcher->apoll_size].events = POLLIN | POLLOUT;
    watcher->apb[watcher->apoll_size] = pb;
    watcher->atimer[watcher->apoll_size] = timer;
    ++watcher->apoll_size;
}


static void remove_socket(struct SocketWatcherData *watcher, pubnub_t *pb)
{
    size_t i;
    for (i = 0; i < watcher->apoll_size; ++i) {
        if (watcher->apoll[i].fd == pb->pal.socket) {
            size_t to_move = watcher->apoll_size - i - 1;
            PUBNUB_ASSERT(watcher->apb[i] == pb);
            CloseHandle(watcher->atimer[i]);
            if (to_move > 0) {
                memmove(watcher->apoll + i, watcher->apoll + i + 1, sizeof watcher->apoll[0] * to_move);
                memmove(watcher->apb + i, watcher->apb + i + 1, sizeof watcher->apb[0] * to_move);
                memmove(watcher->atimer + i, watcher->atimer + i + 1, sizeof watcher->atimer[0] * to_move);
            }
            --watcher->apoll_size;
            break;
        }
    }
}


void socket_watcher_thread(void *arg)
{
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
                DEBUG_PRINTF("poll size = %d, error = %d\n", m_watcher.apoll_size, WSAGetLastError());
            }
            else if (rslt > 0) {
                size_t i;
                for (i = 0; i < m_watcher.apoll_size; ++i) {
                    if (m_watcher.apoll[i].revents & (POLLIN | POLLOUT)) {
                        pbnc_fsm(m_watcher.apb[i]);
                    }
                }
            }
        }

        {
            DWORD rslt = WaitForMultipleObjects(m_watcher.apoll_size, m_watcher.atimer, FALSE, ms);
            if ((rslt >= WAIT_OBJECT_0) && (rslt < WAIT_OBJECT_0 + m_watcher.apoll_size)) {
                pubnub_t *pbp = m_watcher.apb[rslt - WAIT_OBJECT_0];
                pbnc_stop(pbp, PNR_TIMEOUT);
                pbnc_fsm(pbp);
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
    HANDLE timer = CreateWaitableTimer(NULL, TRUE, NULL);

    EnterCriticalSection(&m_watcher.mutw);
    if (timer != NULL) {
        LARGE_INTEGER due;
        due.QuadPart = -10000LL * pb->transaction_timeout_ms;
        SetWaitableTimer(timer, &due, 0, NULL, NULL, 0);
    }
    save_socket(&m_watcher, pb, timer);
    pb->options.use_blocking_io = false;
    pbpal_set_blocking_io(pb);
    
    LeaveCriticalSection(&m_watcher.mutw);

    SetEvent(m_watcher.condw);

    return +1;
}


void pbntf_lost_socket(pubnub_t *pb, pb_socket_t socket)
{
    EnterCriticalSection(&m_watcher.mutw);
    remove_socket(&m_watcher, pb);
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
