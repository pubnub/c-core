/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "posix/monotonic_clock_get_time.h"

void pb_sleep_ms(unsigned long milliseconds)
{
    struct timespec sleep_time;
    sleep_time.tv_sec = milliseconds / UNIT_IN_MILLI;
    sleep_time.tv_nsec = (milliseconds % UNIT_IN_MILLI) * MILLI_IN_NANO;
    nanosleep(&sleep_time, NULL);
}
