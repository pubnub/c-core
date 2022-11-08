/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_sync.h"

#include "core/pubnub_helper.h"
#include "core/pubnub_timers.h"

#include <stdio.h>
#include <time.h>


static void sync_sample_free(pubnub_t* p)
{
    if (PN_CANCEL_STARTED == pubnub_cancel(p)) {
        enum pubnub_res pnru = pubnub_await(p);
        if (pnru != PNR_OK) {
            printf("Awaiting cancel failed: %d('%s')\n",
                   pnru,
                   pubnub_res_2_string(pnru));
        }
    }
    if (pubnub_free(p) != 0) {
        printf("Failed to free the Pubnub context\n");
    }
}


int main()
{
    const int my_retry_limit = 5;
    int i;
    char const *msg = "\"Hello world from publish retry!\"";
    enum pubnub_res res;
    char const *chan = "hello_world";
    pubnub_t *pbp = pubnub_alloc();

    if (NULL == pbp) {
        printf("Failed to allocate Pubnub context!\n");
        return -1;
    }
    pubnub_init(pbp, "demo", "demo");
    pubnub_set_user_id(pbp, "demo");

    pubnub_set_transaction_timeout(pbp, PUBNUB_DEFAULT_NON_SUBSCRIBE_TIMEOUT);
    
    puts("Publishing...");

//! [Publish retry]
    for (i = 0; i < my_retry_limit; ++i) {
        res = pubnub_publish(pbp, chan, msg);
        if ((res != PNR_STARTED) && (res != PNR_OK)) {
            printf("pubnub_publish() returned unexpected: %d('%s')\n", res, pubnub_res_2_string(res));
            sync_sample_free(pbp);
            return -1;
        }

        res = pubnub_await(pbp);
        switch (pubnub_should_retry(res)) {
        case pbccFalse:
            break;
        case pbccTrue:
            printf("Publishing failed with code: %d('%s')\nRetrying...\n",
                   res,
                   pubnub_res_2_string(res));
            continue;
        case pbccNotSet:
            // it's up to you... to be on the safe side:
            puts("Publish failed, but we decided not to retry");
            break;
        }

        if (PNR_OK == res) {
            printf("Published! Response from Pubnub: %s\n", pubnub_last_publish_result(pbp));
        }
        else if (PNR_PUBLISH_FAILED == res) {
            printf("Published failed on Pubnub, description: %s\n", pubnub_last_publish_result(pbp));
        }
        else {
            printf("Publishing failed with code: %d('%s')\n", res, pubnub_res_2_string(res));
        }

        break;
    }
//! [Publish retry]

    /* We're done, but, if keep-alive is on, we can't free,
       we need to cancel first...
     */
    sync_sample_free(pbp);

    puts("Pubnub sync publish retry demo over.");

    return 0;
}
