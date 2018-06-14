/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_callback.h"

#include "core/pubnub_callback_subscribe_loop.h"
#include "core/pubnub_helper.h"
#include "core/pubnub_free_with_timeout.h"

#include <stdio.h>
#include <time.h>


static void subloop_callback(pubnub_t* pbp, char const* message, enum pubnub_res result)
{
    PUBNUB_UNUSED(pbp);
    if (PNR_OK == result) {
        printf("Received message '%s'\n", message);
    }
    else {
        printf("Subscribe failed with code: %d\n", result);
    }
}


static void wait_seconds(unsigned time_in_seconds)
{
    clock_t  start = clock();
    unsigned time_passed_in_seconds;
    do {
        time_passed_in_seconds = (clock() - start) / CLOCKS_PER_SEC;
    } while (time_passed_in_seconds < time_in_seconds);
}


int main()
{
    const unsigned    minutes_in_loop = 1;
    char const*       chan            = "hello_world";
    pubnub_t*         pbp             = pubnub_alloc();
    pubnub_subloop_t* pbsld;

    if (NULL == pbp) {
        printf("Failed to allocate Pubnub context!\n");
        return -1;
    }
    pubnub_init(pbp, "demo", "demo");

    //! [Define subscribe loop]
    pbsld = pubnub_subloop_define(
        pbp, chan, pubnub_subscribe_defopts(), subloop_callback);
    if (NULL == pbsld) {
        printf("Defining a subscribe loop failed\n");
        pubnub_free(pbp);
        /* Waits until the context is released from the processing queue */
        wait_seconds(1);
        return -1;
    }
    //! [Define subscribe loop]

    printf("Entering subscribe loop for channel '%s' for %d minutes...\n",
           chan,
           minutes_in_loop);

    //! [Start a subscribe loop]
    pubnub_subloop_start(pbsld);
    //! [Start Subscribe loop]

    wait_seconds(minutes_in_loop * 60);

    //! [Stop a subscribe loop]
    pubnub_subloop_stop(pbsld);
    //! [Start Subscribe loop]

    //! [Release a subscribe loop]
    pubnub_subloop_undef(pbsld);
    //! [Release Subscribe loop]

    /* We're done, but, if keep-alive is on, we can't free,
       we need to cancel first...
     */
    pubnub_cancel(pbp);
    if (0 != pubnub_free_with_timeout(pbp, 1000)) {
        puts("Failed to free the context in due time");
    }
    else {
        /* Waits until the context is released from the processing queue */
        wait_seconds(1);
    }
    puts("Pubnub callback subloop demo over.");

    return 0;
}
