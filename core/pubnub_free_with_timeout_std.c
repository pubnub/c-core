/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#include "pubnub_free_with_timeout.h"
#include "pubnub_alloc.h"
#include "pubnub_assert.h"
#include "pubnub_log.h"

#include "time.h"



int pubnub_free_with_timeout(pubnub_t* pbp, unsigned millisec)
{
    const clock_t t0 = clock();
    const clock_t clocks_till_timeout = (millisec * CLOCKS_PER_SEC) / 1000;

    PUBNUB_ASSERT_OPT(pbp != NULL);

    while (pubnub_free(pbp) != 0) {
        const clock_t elapsed = clock() - t0;
        if (elapsed  > clocks_till_timeout) {
            PUBNUB_LOG_ERROR("Failed to free the context in %u milli seconds\n", millisec);
            return -1;
        }
    }
    PUBNUB_LOG_TRACE("Freed the context in %lf seconds\n",  ((float)clock() - t0) / CLOCKS_PER_SEC);

    return 0;
}
