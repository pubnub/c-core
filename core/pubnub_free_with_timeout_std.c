/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#include "pubnub_free_with_timeout.h"
#include "pubnub_alloc.h"
#include "pubnub_assert.h"
#include "lib/msstopwatch/msstopwatch.h"
#include "core/pb_sleep_ms.h"
#if PUBNUB_USE_LOGGER
#include "pbcc_logger_manager.h"
#endif // PUBNUB_USE_LOGGER

#define PUBNUB_FREE_POLL_INTERVAL_MS 1


int pubnub_free_with_timeout(pubnub_t* pbp, unsigned millisec)
{
    const pbmsref_t t0 = pbms_start();

    PUBNUB_ASSERT_OPT(pbp != NULL);

    while (pubnub_free(pbp) != 0) {
        const pbms_t elapsed = pbms_elapsed(t0);
        if (elapsed > (pbms_t)millisec) {
            PUBNUB_LOG_ERROR(
                pbp, "Failed to free the context in %u milliseconds", millisec);
            return -1;
        }
        pb_sleep_ms(PUBNUB_FREE_POLL_INTERVAL_MS);
    }
    PUBNUB_LOG_TRACE(
        pbp,
        "Freed the context in %lf seconds",
        ((float)clock() - t0) / CLOCKS_PER_SEC);

    return 0;
}
