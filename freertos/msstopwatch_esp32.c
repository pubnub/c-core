/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#include "lib/msstopwatch/msstopwatch.h"
#include "freertos/pbtimespec_elapsed_ms_esp32.h"
#include "pubnub_assert.h"
#include <sys/time.h>

/** This uses our helper module that provides a monotonic
    clock like POSIX clock_gettime(CLOCK_MONOTONIC,...);
*/

static struct timespec msclock(void)
{
    struct timespec ts;
    gettimeofday((struct timeval*)&ts, NULL);
    
    return ts;
}


pbmsref_t pbms_start(void)
{
    pbmsref_t result = { msclock() };
    return result;
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
    PUBNUB_ASSERT(pbms_active(since));
    return pbtimespec_elapsed_ms(since.t_ref, msclock());
}
