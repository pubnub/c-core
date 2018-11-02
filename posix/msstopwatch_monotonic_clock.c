/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#include "lib/msstopwatch/msstopwatch.h"

#include "core/pubnub_assert.h"

#include "posix/monotonic_clock_get_time.h"


/** This uses our helper module that provides a monotonic
    clock like POSIX clock_gettime(CLOCK_MONOTONIC,...);
*/

static int32_t msclock(void)
{
    struct timespec ts;
    if (0 == monotonic_clock_get_time(&ts)) {
        return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    }
    else {
        return -1;
    }
}


pbmsref_t pbms_start(void)
{
    pbmsref_t rslt = { msclock() };
    return rslt;
}


void pbms_stop(pbmsref_t* psw)
{
    psw->t_ref = -1;
}


bool pbms_active(pbmsref_t stopwatch)
{
    return stopwatch.t_ref > -1;
}


pbms_t pbms_elapsed(pbmsref_t since)
{
    PUBNUB_ASSERT(pbms_active(since));
    return msclock() - since.t_ref;
}
