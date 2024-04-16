/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#include "lib/msstopwatch/msstopwatch.h"

// TODO: whole file is a stub!

/** This uses our helper module that provides a monotonic
    clock like POSIX clock_gettime(CLOCK_MONOTONIC,...);
*/

static struct timespec msclock(void)
{
    return *(struct timespec*)0;
}


pbmsref_t pbms_start(void)
{
    return *(pbmsref_t*)0;
}


void pbms_stop(pbmsref_t* psw)
{
    psw->t_ref.tv_nsec = -1;
}


bool pbms_active(pbmsref_t stopwatch)
{
    return stopwatch.t_ref.tv_nsec > -1;
}


pbms_t pbms_elapsed(pbmsref_t since)
{
    return 0;
}
