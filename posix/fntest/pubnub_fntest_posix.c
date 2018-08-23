/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#include "core/fntest/pubnub_fntest.h"

#include <time.h>
#include <stdlib.h>
#include <stdio.h>


enum TimerState {
    tmrIdle,
    tmrRunning,
    tmrExpired
};

struct pnfntst_timer {
    unsigned total_ms;
    time_t t0;
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
    t->total_ms = ms;
    t->t0 = time(NULL);
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
    t->t0 = time(NULL);
    t->state = tmrRunning;

    return 0;
}


bool pnfntst_timer_is_running(pnfntst_timer_t *t)
{
    if (tmrRunning == t->state) {
        double diff = difftime(time(NULL), t->t0);
        if (diff * 1000 > t->total_ms) {
            t->state = tmrExpired;
        }
    }
    return tmrRunning == t->state;
}


void pnfntst_free_timer(void* t)
{
    free(t);
}
