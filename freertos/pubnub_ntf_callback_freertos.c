/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_ntf_callback.h"

#if PUBNUB_USE_RETRY_CONFIGURATION
#include "core/pubnub_retry_configuration_internal.h"
#include "core/pubnub_pubsubapi.h"
#endif // #if PUBNUB_USE_RETRY_CONFIGURATION

#include "pubnub_internal.h"
#include "pubnub_assert.h"
#include "pubnub_log.h"
#include "pbntf_trans_outcome_common.h"
#include "pubnub_timer_list.h"

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
#if PUBNUB_TIMERS_API
    pubnub_t *timer_head;
#endif
    SemaphoreHandle_t queue_lock;
    size_t queue_size;
    size_t queue_head;
    size_t queue_tail;
    pubnub_t *queue_apb[128];
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

static int elapsed_ms(TickType_t prev, TickType_t now)
{
    return (now - prev);
}


int pbntf_watch_in_events(pubnub_t *pbp)
{
    /* not supported under FreeRTOS+TCP for now, though we could
        try with eSELECT_ALL/eSELECT_READ/eSELECT_WRITE
    */
    return 0;
}


int pbntf_watch_out_events(pubnub_t *pbp)
{
    /* not supported under FreeRTOS+TCP for now, though we could
        try with eSELECT_ALL/eSELECT_READ/eSELECT_WRITE
    */
    return 0;
}


void socket_watcher_task(void *arg)
{
    TickType_t xTimePrev = xTaskGetTickCount();
    struct SocketWatcherData *pWatcher = (struct SocketWatcherData *)arg;

    for (;;) {

        ulTaskNotifyTake(pdTRUE, TICKS_TO_WAIT);

        if (pdFALSE == xSemaphoreTakeRecursive(m_watcher.queue_lock, TICKS_TO_WAIT)) {
            continue;
        }
        while (m_watcher.queue_head != m_watcher.queue_tail) {
            pubnub_t *pbp = m_watcher.queue_apb[m_watcher.queue_tail++];
            if (m_watcher.queue_tail == m_watcher.queue_size) {
                m_watcher.queue_tail = 0;
            }
            xSemaphoreGiveRecursive(m_watcher.queue_lock);
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
            if (pdFALSE == xSemaphoreTakeRecursive(m_watcher.queue_lock, TICKS_TO_WAIT)) {
                break;
            }
            if (m_watcher.queue_tail == m_watcher.queue_size) {
                m_watcher.queue_tail = 0;
            }
        }
        xSemaphoreGiveRecursive(m_watcher.queue_lock);
 
        if (pdFALSE == xSemaphoreTakeRecursive(m_watcher.mutw, TICKS_TO_WAIT)) {
            continue;
        }

        if (pWatcher->apb_size > 0) {
            if (FreeRTOS_select(pWatcher->xFD_set, TICKS_TO_WAIT) != 0) {
                pubnub_t **ppbp;
                for (ppbp = pWatcher->apb; ppbp < pWatcher->apb + pWatcher->apb_size; ++ppbp) {
                    if (FreeRTOS_FD_ISSET((*ppbp)->pal.socket, pWatcher->xFD_set)) {
                        pbntf_requeue_for_processing(*ppbp);
                    }
                }
            }
        }

        if (PUBNUB_TIMERS_API) {
            TickType_t xTimeNow = xTaskGetTickCount();
            int elapsed = elapsed_ms(xTimePrev, xTimeNow);
            if (elapsed > 0) {
                pubnub_t *expired = pubnub_timer_list_as_time_goes_by(&m_watcher.timer_head, elapsed);
                while (expired != NULL) {
                    pubnub_t *next;
                    
                    pubnub_mutex_lock(expired->monitor);

                    next = expired->next;

                    pbnc_stop(expired, PNR_TIMEOUT);
                    
                    expired->previous = NULL;
                    expired->next = NULL;
                    pubnub_mutex_lock(expired->monitor);

                    expired = next;
                }
                xTimePrev = xTimeNow;
            }
        }

        xSemaphoreGiveRecursive(m_watcher.mutw);
    }
}


#define SOCKET_WATCHER_STACK_DEPTH 512
#define PUBNUB_TASK_PRIORITY 2


int pbntf_init(pubnub_t* pb)
{
    PUBNUB_UNUSED(pb);

    m_watcher.mutw = xSemaphoreCreateRecursiveMutex();
    if (NULL == m_watcher.mutw) {
        return -1;
    }
    
    m_watcher.queue_size = sizeof m_watcher.queue_apb / sizeof m_watcher.queue_apb[0];
    m_watcher.queue_head = m_watcher.queue_tail = 0;

    m_watcher.queue_lock = xSemaphoreCreateRecursiveMutex();
    if (NULL == m_watcher.queue_lock) {
        vSemaphoreDelete(m_watcher.mutw);
        return -1;
    }
        
    m_watcher.xFD_set = FreeRTOS_CreateSocketSet();
    if (NULL == m_watcher.xFD_set) {
        vSemaphoreDelete(m_watcher.queue_lock);
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
        vSemaphoreDelete(m_watcher.queue_lock);
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
    if (PUBNUB_TIMERS_API) {
        m_watcher.timer_head = pubnub_timer_list_add(m_watcher.timer_head, pb);
    }
    
    xSemaphoreGiveRecursive(m_watcher.mutw);

    xTaskNotifyGive(m_watcher.task);

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
    PUBNUB_UNUSED(socket);
    if (pdFALSE == xSemaphoreTakeRecursive(m_watcher.mutw, TICKS_TO_WAIT)) {
        return ;
    }
    remove_socket(&m_watcher, pb);
    remove_timer_safe(pb);

    xSemaphoreGiveRecursive(m_watcher.mutw);
    xTaskNotifyGive(m_watcher.task);
}


void pbntf_update_socket(pubnub_t *pb, pb_socket_t socket)
{
    PUBNUB_UNUSED(socket);

    if (pdFALSE == xSemaphoreTakeRecursive(m_watcher.mutw, TICKS_TO_WAIT)) {
        return ;
    }

    /* TODO: Actually, this is not fully correct. We should also remove the old
        socket from the socket set, but, here we don't have the info "what was
        the old socket". OTOH, this updating of the socket will probably never
        happen on FreeRTOS+TCP and is not a functional problem, as even if
        something happens with the old socket, it will be ingored. So it's just
        a small performance penalty.
        */
    FreeRTOS_FD_SET(pb->pal.socket, m_watcher.xFD_set, eSELECT_ALL);

    xSemaphoreGiveRecursive(m_watcher.mutw);
    xTaskNotifyGive(m_watcher.task);
}



void pbntf_trans_outcome(pubnub_t *pb)
{
    PBNTF_TRANS_OUTCOME_COMMON(pb);
#if PUBNUB_USE_RETRY_CONFIGURATION
    if (NULL != pb->core.retry_configuration &&
        pubnub_retry_configuration_retryable_result_(pb)) {
        uint16_t delay = pubnub_retry_configuration_delay_(pb);

        if (delay > 0) {
            if (NULL == pb->core.retry_timer)
                pb->core.retry_timer = pbcc_request_retry_timer_alloc(pb);
            if (NULL != pb->core.retry_timer) {
                pbcc_request_retry_timer_start(pb->core.retry_timer, delay);
                return;
            }
        }
    }

    /** There were no need to start retry timer, we can free it if exists. */
    if (NULL != pb->core.retry_timer) {
        pb->core.http_retry_count = 0;
        pbcc_request_retry_timer_free(&pb->core.retry_timer);
    }
#endif // #if PUBNUB_USE_RETRY_CONFIGURATION
    if (pb->cb != NULL) {
        pb->cb(pb, pb->trans, pb->core.last_result, pb->user_data);
    }
}


int pbntf_enqueue_for_processing(pubnub_t *pb)
{
    int result;
    size_t next_head;

    PUBNUB_ASSERT_OPT(pb != NULL);

    if (pdFALSE == xSemaphoreTakeRecursive(m_watcher.queue_lock, TICKS_TO_WAIT)) {
        return -1;
    }
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
    xSemaphoreGiveRecursive(m_watcher.mutw);
    xTaskNotifyGive(m_watcher.task);

    return result;
}


int pbntf_requeue_for_processing(pubnub_t *pb)
{
    bool found = false;
    size_t i;

    PUBNUB_ASSERT_OPT(pb != NULL);

    if (pdFALSE == xSemaphoreTakeRecursive(m_watcher.queue_lock, TICKS_TO_WAIT)) {
        return -1;
    }
    for (i = m_watcher.queue_tail; i != m_watcher.queue_head; i = (((i + 1) == m_watcher.queue_size) ? 0 : i + 1)) {
        if (m_watcher.queue_apb[i] == pb) {
            found = true;
            break;
        }
    }
    xSemaphoreGiveRecursive(m_watcher.queue_lock);
    xTaskNotifyGive(m_watcher.task);

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
