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
    WSAPOLLFD *apoll;
    size_t apoll_size;
    size_t apoll_cap;
    pubnub_t **apb;
    CRITICAL_SECTION mutw;
    HANDLE condw;
    uintptr_t thread_id;
};

static struct SocketWatcherData m_watcher;


static void save_socket(struct SocketWatcherData *watcher, pubnub_t *pb)
{
    if (watcher->apoll_size == watcher->apoll_cap) {
        WSAPOLLFD *npalloc = (WSAPOLLFD*)realloc(watcher->apoll, sizeof watcher->apoll[0] * (watcher->apoll_size+1));
        pubnub_t **npapb = (pubnub_t**)realloc(watcher->apb, sizeof watcher->apb[0] * (watcher->apoll_size+1));
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
        watcher->apoll[watcher->apoll_size].fd = pb->pal.socket;
        watcher->apoll[watcher->apoll_size].events = POLLIN | POLLOUT;
        watcher->apb[watcher->apoll_size] = pb;
        watcher->apoll_cap = ++watcher->apoll_size;
    }
    else {
        watcher->apoll[watcher->apoll_size].fd = pb->pal.socket;
        watcher->apoll[watcher->apoll_size].events = POLLIN | POLLOUT;
        watcher->apb[watcher->apoll_size] = pb;
        ++watcher->apoll_size;
    }
}


static void remove_socket(struct SocketWatcherData *watcher, pubnub_t *pb)
{
    size_t i;
    for (i = 0; i < watcher->apoll_size; ++i) {
        if (watcher->apoll[i].fd == pb->pal.socket) {
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


void socket_watcher_thread(void *arg)
{
    for (;;) {
        int rslt;
        DWORD ms = 200;
        
        WaitForSingleObject(m_watcher.condw, ms);
        
        EnterCriticalSection(&m_watcher.mutw);
        if (0 == m_watcher.apoll_size) {
            LeaveCriticalSection(&m_watcher.mutw);
            continue;
        }
        rslt = WSAPoll(m_watcher.apoll, m_watcher.apoll_size, ms);
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
    pb->use_blocking_io = false;
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
    return pb>user_data;
}
