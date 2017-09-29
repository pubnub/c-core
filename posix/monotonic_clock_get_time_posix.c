/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "monotonic_clock_get_time.h"


int monotonic_clock_get_time(struct timespec *tp)
{
    return clock_gettime(CLOCK_MONOTONIC, tp);
}
