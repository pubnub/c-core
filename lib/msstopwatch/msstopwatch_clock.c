/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#include "lib/msstopwatch/msstopwatch.h"

#include <time.h>


/** This is the default implementation, relying on the standard
    clock() function, which is most probably _incorrect_, as it
    often returns only the CPU clock, which is still while the
    CPU is asleep.
*/

pbmsref_t pbms_start(void)
{
  pbmsref_t rslt = {(clock() * 1000) / CLOCKS_PER_SEC } ;
  return rslt;
}


pbms_t pbms_elapsed(pobmsref_t since)
{
  return (clock() * 1000) / CLOCKS_PER_SEC - since.t_ref;
}

