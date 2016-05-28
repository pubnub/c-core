/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_ntf_callback.h"

#include "pubnub_internal.h"
#include "pubnub_assert.h"
#include "pbntf_trans_outcome_common.h"
#include "pubnub_timer_list.h"
#include "pubnub_log.h"

#include "pbpal.h"

#include "sys_tmr.h"

#include <stdlib.h>
#include <string.h>


struct SocketWatcherData {
    pubnub_t **apb;
    size_t apb_size;
    size_t apb_cap;
    pubnub_t *timer_head;
};

static struct SocketWatcherData m_watcher;

static size_t find_pbp(struct SocketWatcherData *watcher, pubnub_t *pb)
{
    size_t i;
    for (i = 0; i < watcher->apb_size; ++i) {
        if (watcher->apb[i] == pb) {
            break;
        }
    }
    return i;
}

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

    watcher->apb[watcher->apb_size++] = pb;
}


static void remove_socket(struct SocketWatcherData *watcher, pubnub_t *pb)
{
    size_t i = find_pbp(watcher, pb);
    if (i < watcher->apb_size) {
        size_t to_move = watcher->apb_size - i - 1;
        if (to_move > 0) {
            memmove(watcher->apb + i, watcher->apb + i + 1, sizeof watcher->apb[0] * to_move);
        }
        --watcher->apb_size;
    }
}

static int elapsed_ms(uint32_t prev, uint32_t now)
{
    uint32_t freq = SYS_TMR_TickCounterFrequencyGet();
    return (freq != 0) ? (1000 * (now - prev)) / freq : 0;
}


void pubnub_task(void)
{
    static uint32_t s_tick_prev;

    if (0 == s_tick_prev) {
        s_tick_prev = SYS_TMR_TickCountGet();
    }
    
    if (m_watcher.apb_size > 0) {
        pubnub_t **ppbp;
        uint32_t tick_now = SYS_TMR_TickCountGet();
        int elapsed = elapsed_ms(s_tick_prev, tick_now);

        for (ppbp = m_watcher.apb; ppbp < m_watcher.apb + m_watcher.apb_size; ++ppbp) {
            pbnc_fsm(*ppbp);
        }
        if (elapsed > 0) {
            pubnub_t *expired = pubnub_timer_list_as_time_goes_by(&m_watcher.timer_head, elapsed);
            while (expired != NULL) {
                pubnub_t *next = expired->next;

                pbnc_stop(expired, PNR_TIMEOUT);

                expired->previous = NULL;
                expired->next = NULL;
                expired = next;
            }
            s_tick_prev = tick_now;
        }
    }
}


int pbntf_init(void)
{
    return 0;
}


int pbntf_got_socket(pubnub_t *pb, pb_socket_t socket)
{
    size_t i = find_pbp(&m_watcher, pb);
    PUBNUB_ASSERT_OPT(i == m_watcher.apb_size);
    if (i == m_watcher.apb_size) {
        save_socket(&m_watcher, pb);
        if (PUBNUB_TIMERS_API) {
            m_watcher.timer_head = pubnub_timer_list_add(m_watcher.timer_head, pb);
        }
    }
    return +1;
}

void pbntf_update_socket(pubnub_t *pb, pb_socket_t socket)
{
    /* We don't care. */
    PUBNUB_UNUSED(pb);
    PUBNUB_UNUSED(socket);
}

static void remove_timer_safe(pubnub_t *to_remove)
{
    if ((to_remove->previous != NULL) || (to_remove->next != NULL) 
        || (to_remove == m_watcher.timer_head)) {
        m_watcher.timer_head = pubnub_timer_list_remove(m_watcher.timer_head, to_remove);
    }
}

void pbntf_lost_socket(pubnub_t *pb, pb_socket_t socket)
{
    PUBNUB_UNUSED(socket);
    remove_socket(&m_watcher, pb);
    remove_timer_safe(pb);
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
