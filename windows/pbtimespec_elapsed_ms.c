/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbtimespec_elapsed_ms.h"

/** The number of Windows FILETIME intervals in a millisecond. Windows
    FILETIME interval is 100 ns.  In practice, the actual resolution
    may be (much) different, but, nominally it's 100ns.
*/
#define MSEC_IN_FILETIME_INTERVALS (10 * 1000)


int pbtimespec_elapsed_ms(FILETIME prev_timspec, FILETIME timspec)
{
    ULARGE_INTEGER prev;
    ULARGE_INTEGER current;
    prev.LowPart     = prev_timspec.dwLowDateTime;
    prev.HighPart    = prev_timspec.dwHighDateTime;
    current.LowPart  = timspec.dwLowDateTime;
    current.HighPart = timspec.dwHighDateTime;
    return (int)((current.QuadPart - prev.QuadPart) / MSEC_IN_FILETIME_INTERVALS);
}
