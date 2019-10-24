/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if !defined(INC_MSSTOPWATCH)
#define INC_MSSTOPWATCH


/** @file msstopwatch.h

    A milisecond stopwatch. It's not milisecond accurate, it's just
    the resolution. It might be as bad as 100ms accurate, but, we
    "trade in milliseconds", as a "common denominator".

    We actually want to measure things in seconds, up to several
    minutes, but no more than few hours.

    Such interface is highly "portable", that is, should be easy
    to implement on any platform.

    Keep in mind that this isn't covered by the C standard library.
    clock() can measure only the _CPU_ time (i.e. might not take into
    account that CPU "goes to sleep"), while time() measures "wall
    clock time", thus might "jump around" on time adjustment for
    synchronization or daylight savings or other administrative
    measures.

*/
#include "msstopwatch_pal.h"

#include <stdint.h>
#include <stdbool.h>


typedef int32_t pbms_t;

/** Starts a stopwatch. The result has no particular meaning, 
    is only to be used as the parameter of pbms_elapsed().
    @return a "reference" point in time
*/
pbmsref_t pbms_start(void);

/** Stops the given stopwatch. This basically just marks it so that we
    know not to use it with pbms_elapsed().
 */
void pbms_stop(pbmsref_t* psw);

/** Returns whether the given @p stopwatch is active or not.
    A stopwatch is not active if it has been stopped via
    pbms_stop().
*/
bool pbms_active(pbmsref_t stopwatch);

/** Returns the number of milliseconds elapsed @p since the 
    time reference established with pbms_start().

    This might be negative, if you "artificially" create @p since,
    which you are strongly encouraged _not_ to.

    @precondition pbms_active(since)
*/
pbms_t pbms_elapsed(pbmsref_t since);


#endif /* !defined(INC_MSSTOPWATCH) */
