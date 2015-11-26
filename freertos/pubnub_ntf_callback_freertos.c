/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_ntf_callback.h"

#include "pubnub_internal.h"
#include "pubnub_assert.h"
#include "pbntf_trans_outcome_common.h"

#include "pbpal.h"

#include "semphr.h"

#include <stdlib.h>
#include <string.h>

struct SocketWatcherData {
    pubnub_t **apb;
    size_t apb_size;
    size_t apb_cap;
    xSocketSet_t xFD_set;
    SemaphoreHandle_t  mutw;
    TaskHandle_t task;
};

static struct SocketWatcherData m_watcher;


#define TICKS_TO_WAIT pdMS_TO_TICKS(200)


static void save_socket(struct SocketWatcherData *watcher, pubnub_t *pb)
{
    if (watcher->apb_size == watcher->apb_cap) {
        size_t newcap = watcher->apb_cap + 2;
        pubnub_t **npapb = (pubnub_t**)realloc(watcher->apb, sizeof watcher->apb[0] * newcap);
        if (NULL == npapb) {
            return;
        }
        watcher->apb = npapb;
        watcher->apb_cap = newcap;
    }

    FreeRTOS_FD_SET(pb->pal.socket, watcher->xFD_set, eSELECT_ALL);

    watcher->apb[watcher->apb_size] = pb;
    ++watcher->apb_size;
}


static void remove_socket(struct SocketWatcherData *watcher, pubnub_t *pb)
{
    size_t i;
    for (i = 0; i < watcher->apb_size; ++i) {
        if (watcher->apb[i] == pb) {
            size_t to_move = watcher->apb_size - i - 1;
            FreeRTOS_FD_CLR(pb->pal.socket, watcher->xFD_set, eSELECT_ALL);
            if (to_move > 0) {
                memmove(watcher->apb + i, watcher->apb + i + 1, sizeof watcher->apb[0] * to_move);
            }
            --watcher->apb_size;
            break;
        }
    }
}


void socket_watcher_task(void *arg)
{
    struct SocketWatcherData *pWatcher = (struct SocketWatcherData *)arg;

    for (;;) {

        ulTaskNotifyTake(pdTRUE, TICKS_TO_WAIT);

        if (pdFALSE == xSemaphoreTakeRecursive(m_watcher.mutw, TICKS_TO_WAIT)) {
            continue;
        }

        if (pWatcher->apb_size > 0) {
            if (FreeRTOS_select(pWatcher->xFD_set, TICKS_TO_WAIT) != 0) {
                pubnub_t **ppbp;
                for (ppbp = pWatcher->apb; ppbp < pWatcher->apb + pWatcher->apb_size; ++ppbp) {
                    if (FreeRTOS_FD_ISSET((*ppbp)->pal.socket, pWatcher->xFD_set)) {
                        pbnc_fsm(*ppbp);
                    }
                }
            }
        }
        xSemaphoreGiveRecursive(m_watcher.mutw);
    }
}


#define SOCKET_WATCHER_STACK_DEPTH 512
#define PUBNUB_TASK_PRIORITY 2


int pbntf_init(void)
{
    m_watcher.mutw = xSemaphoreCreateRecursiveMutex();
    if (NULL == m_watcher.mutw) {
        return -1;
    }
    
    m_watcher.xFD_set = FreeRTOS_CreateSocketSet();
    if (NULL == m_watcher.xFD_set) {
        vSemaphoreDelete(m_watcher.mutw);
        return -1;
    }
    if (pdPASS != xTaskCreate(
        socket_watcher_task, 
        "Pubnub socket watcher", 
        SOCKET_WATCHER_STACK_DEPTH, 
        &m_watcher, 
        PUBNUB_TASK_PRIORITY, 
        &m_watcher.task
        )) {
        vSemaphoreDelete(m_watcher.mutw);
        FreeRTOS_DeleteSocketSet(m_watcher.xFD_set);
        return -1;
    }
    return 0;
}


int pbntf_got_socket(pubnub_t *pb, pb_socket_t socket)
{
    PUBNUB_UNUSED(socket);
    if (pdFALSE == xSemaphoreTakeRecursive(m_watcher.mutw, TICKS_TO_WAIT)) {
        return -1;
    }
     
    save_socket(&m_watcher, pb);
    
    xSemaphoreGiveRecursive(m_watcher.mutw);

    xTaskNotifyGive(m_watcher.task);

    return +1;
}


void pbntf_lost_socket(pubnub_t *pb, pb_socket_t socket)
{
    PUBNUB_UNUSED(socket);
    if (pdFALSE == xSemaphoreTakeRecursive(m_watcher.mutw, TICKS_TO_WAIT)) {
        return ;
    }
    remove_socket(&m_watcher, pb);
    xSemaphoreGiveRecursive(m_watcher.mutw);
    xTaskNotifyGive(m_watcher.task);
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
