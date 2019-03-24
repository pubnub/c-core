#include "srand_from_pubnub_time.h"

#include "pubnub_internal.h"
#include "pubnub_pubsubapi.h"
#include "pubnub_coreapi.h"
#include "pubnub_ntf_sync.h"
#include "pubnub_assert.h"

#include <stdlib.h>
#include <string.h>


/** This is not completely accurate, but is good enough:
    max unsinged int for most common sizes of unsigned integer:

    16-bits -> 5
    32-bits -> 10
    64-bits -> 19 (we calculate 20)
    128-bits -> 38 (we calculate 40)

    But, the thing is, for our purpose, it doesn't matter that we're
    not so accurate after 32-bits, as the number of digits in Pubnub
    time is 17 at this time and it will take a _long_ time before it
    reaches 19.
 */
#define MAX_UINT_NUM_OF_DIGITS ((5 * sizeof(unsigned)) / 2)


int srand_from_pubnub_time(pubnub_t* pbp)
{
    char const*     pbtime;
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pbp));

    rslt = pubnub_time(pbp);
    if (rslt == PNR_STARTED) {
        rslt = pubnub_await(pbp);
    }
    if (rslt != PNR_OK) {
        return -1;
    }
    pbtime = pubnub_get(pbp);
    if (pbtime != NULL) {
        size_t      length_of_time = strlen(pbtime);
        char const* s              = pbtime + length_of_time - 1;
        unsigned    val_for_srand  = 0;
        unsigned    i;
        if (0 == length_of_time) {
            return -1;
        }

        for (i = 0; (i < MAX_UINT_NUM_OF_DIGITS) && (s > pbtime); ++i, --s) {
            val_for_srand = val_for_srand * 10 + *s - '0';
        }
        srand(val_for_srand);

        return 0;
    }

    return -1;
}
