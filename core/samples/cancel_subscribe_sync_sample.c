/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_sync.h"

#include "core/pubnub_helper.h"

#include <stdio.h>
#include <stdlib.h>
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
    bool done = false;
    /* This is a widely use channel, something should happen there
       from time to time
    */
    char const* chan = "hello_world";
    pubnub_t*   pbp  = pubnub_alloc();
    if (NULL == pbp) {
        printf("Failed to allocate Pubnub context!\n");
        return -1;
    }

    pubnub_init(pbp, "demo", "demo");
    srand((unsigned)time(NULL));

    /* Using non-blocking I/O is essential, otherwise waiting for
        incoming data will block! We recommend you not enable verbose
        debugging, as it will be filled up with tracing.
    */
    pubnub_set_non_blocking_io(pbp);

    puts("--------------------------");
    puts("Subscribe loop starting...");
    puts("--------------------------");

    while (!done) {
        time_t          t    = time(NULL);
        bool            stop = false;
        enum pubnub_res res  = pubnub_subscribe(pbp, chan, NULL);

        /* Don't await here, 'cause it will loop until done */
        while (!stop) {
            res = pubnub_last_result(pbp);
            if (PNR_STARTED == res) {
                /* Here we simulate the "get out of subscribe loop"
                   external signal with a random number. Basically,
                   this has a 4% chance of stopping the wait every
                   second.
                */
                if (time(NULL) != t) {
                    stop = (rand() % 25) == 3;
                    t    = time(NULL);
                }
            }
            else {
                if (PNR_OK == res) {
                    puts("Subscribed! Got messages:");
                    for (;;) {
                        char const* msg = pubnub_get(pbp);
                        if (NULL == msg) {
                            break;
                        }
                        puts(msg);
                    }
                }
                else {
                    printf("Subscribing failed with code: %d('%s')\n", res, pubnub_res_2_string(res));
                }
                if (time(NULL) != t) {
                    done = (rand() % 25) == 19;
                    t    = time(NULL);
                }
                break;
            }
        }
        if (stop) {
            puts("---------------------------");
            puts("Cancelling the Subscribe...");
            puts("---------------------------");
            pubnub_cancel(pbp);

            /* Now it's OK to await, since we don't have anything else
               to do
            */
            pubnub_await(pbp);
            done = true;
        }
    }

    sync_sample_free(pbp);

    puts("Pubnub cancel subscribe sync demo over.");

    return 0;
}
