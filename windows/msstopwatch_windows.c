/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#include "lib/msstopwatch/msstopwatch.h"

#include "core/pubnub_assert.h"


/** This uses GetTickCount64(), which is very fast, but innacurate -
    typical resolution is ~16 ms, could be better or worse, but
    not much better. This is acceptable for the purposes of this
    module.
*/

pbmsref_t pbms_start(void)
{
    pbmsref_t rslt = { GetTickCount64() + 1 };
    return rslt;
}


void pbms_stop(pbmsref_t* psw)
{
    psw->t_ref = 0;
}


bool pbms_active(pbmsref_t stopwatch)
{
    return stopwatch.t_ref > 0;
}


pbms_t pbms_elapsed(pbmsref_t since)
{
    PUBNUB_ASSERT(pbms_active(since));
    return (pbms_t)(GetTickCount64() + 1 - since.t_ref);
}
