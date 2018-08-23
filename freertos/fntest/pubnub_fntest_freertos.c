/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#include "pubnub_fntest.h"

#include <FreeRTOS.h>


enum TimerState {
    tmrIdle,
    tmrRunning,
    tmrExpired
};

struct pnfntst_timer {
    TickType_t total_ticks;
    TickType_t t0;
    enum TimerState state;
};


pnfntst_timer_t *pnfntst_alloc_timer(void)
{
    pnfntst_timer_t *rslt = malloc(sizeof (struct pnfntst_timer));
    if (NULL == rslt) {
        return NULL;
    }
    rslt->state = tmrIdle;

    return rslt;
}


int pnfntst_start_timer(pnfntst_timer_t *t, unsigned ms)
{
    if (tmrRunning == t->state) {
        return -1;
    }
    t->total_ticks = pdMS_TO_TICKS(ms);
    t->t0 = xTaskGetTickCount();
    t->state = tmrRunning;
    return 0;
}


void pnfntst_stop_timer(pnfntst_timer_t *t)
{
    t->state = tmrIdle;
}


int pnfntst_reset_timer(pnfntst_timer_t *t)
{
    if (tmrIdle == t->state) {
        return -1;
    }
    t->t0 = xTaskGetTickCount();
    t->state = tmrRunning;

    return 0;
}


bool pnfntst_timer_is_running(pnfntst_timer_t *t)
{
    if (tmrRunning == t->state) {
        TickType_t diff = xTaskGetTickCount() - t->t0;
        if (diff > t->total_ticks) {
            t->state = tmrExpired;
        }
    }
    return tmrRunning == t->state;
}


void pnfntst_free_timer(void* t)
{
    free(t);
}
