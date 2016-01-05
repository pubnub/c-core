#if !defined INC_MONOTONIC_CLOCK_GET_TIME
#define      INC_MONOTONIC_CLOCK_GET_TIME

#include <time.h>

/** How many "millis" in a "nano" of some (any) unit. */
#define MILLI_IN_NANO (1000 * 1000)

/** How many "nanos" in one (unit) of something. */
#define UNIT_IN_NANO (1000 * MILLI_IN_NANO)

/** How many "millis" in one (unit) of something. */
#define UNIT_IN_MILLI (1000)

/** This abstracts getting of monotonic clock time. On POSIX compliant
    systems, this is a simple `clock_gettime(CLOCK_MONOTONIC, tp)`.
    But, not all Unix-like OSes are POSIX compliant that way, with OSX
    / Darwin being a well-known one.
 */
int monotonic_clock_get_time(struct timespec *tp);


#endif /* !defined INC_MONOTONIC_CLOCK_GET_TIME */
