/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_sync.h"


#include <stdio.h>
#include <stdlib.h>
#include <time.h>


int main()
{
    /* This is a widely use channel, something should happen there
       from time to time
    */
    char const *chan = "hello_world";
    pubnub_t *pbp = pubnub_alloc();
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

    for (;;) {
        time_t t = time(NULL);
        bool stop = false;
        enum pubnub_res res = pubnub_subscribe(pbp, chan, NULL);
        if (res != PNR_STARTED) {
            printf("pubnub_subscribe() returned unexpected: %d\n", res);
            break;;
        }

        /* Don't await here, 'cause it will loop until done */
        while (!stop) {
            res = pubnub_last_result(pbp);
            if (res == PNR_STARTED) {
                /* Here we simulate the "get out of subscribe loop"
                   external signal with a random number. Basically,
                   this has a 4% chance of stopping the wait every
                   second.
                */
                if (time(NULL) != t) {
                    stop = (rand() % 25) == 3;
                    t = time(NULL);
                }
            }
            else {
                if (PNR_OK == res) {
                    puts("Subscribed! Got messages:");
                    for (;;) {
                        char const *msg = pubnub_get(pbp);
                        if (NULL == msg) {
                            break;
                        }
                        puts(msg);
                    }
                }
                else {
                    printf("Subscribing failed with code: %d\n", res);
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
            break;
        }
    }


    /* We're done */
    if (pubnub_free(pbp) != 0) {
        printf("Failed to free the Pubnub context `pbp`\n");
    }

    puts("Pubnub cancel subscribe sync demo over.");

    return 0;
}
