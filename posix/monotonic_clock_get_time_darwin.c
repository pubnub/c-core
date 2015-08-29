/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "monotonic_clock_get_time.h"

#include <mach/mach.h>
#include <mach/mach_time.h>
#include <unistd.h>


int monotonic_clock_get_time(struct timespec *tp)
{
    static mach_timebase_info_data_t s_time_base_info;
    uint64_t absolute_time = mach_absolute_time();
    uint64_t nanos;
    
    if (0 == s_time_base_info.denom) {
        /* This is obviously not thread safe, but, we don't care, as
           we always call it from the same thread. A thread safe
           variant would either always call it, or provide some kind
           of protection (it is actually hard to foresee which one of
           those approaches would be faster).
        */
        mach_timebase_info(&s_time_base_info);
    }
    nanos = (absolute_time * s_time_base_info.numer) / s_time_base_info.denom;
    tp->tv_sec = nanos / UNIT_IN_NANO;
    tp->tv_nsec = nanos % UNIT_IN_NANO;
    
    return 0;
}
