/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_callback.h"

#include "pubnub_callback_subscribe_loop.h"
#include "pubnub_helper.h"
#include "pubnub_free_with_timeout.h"

#include <stdio.h>
#include <time.h>


static void subloop_callback(char const* message, enum pubnub_res result)
{
    if (PNR_OK == result) {
        printf("Received message '%s'\n", message);
    }
    else {
        printf("Subscribe failed with code: %d\n", result);
    }
}


static void just_wait_minutes(unsigned min)
{
    const unsigned sec = 60 * min;
    time_t t0 = time(NULL);
    for (;;) {
        const double d = difftime(time(NULL), t0);
        if (d > sec) {
            break;
        }
    }
}


int main()
{
    const unsigned minutes_in_loop = 1;
    char const* chan = "hello_world";
    pubnub_t *pbp = pubnub_alloc();
    pubnub_subloop_t* pbsld;

    if (NULL == pbp) {
        printf("Failed to allocate Pubnub context!\n");
        return -1;
    }
    pubnub_init(pbp, "demo", "demo");

//! [Define subscribe loop]
    pbsld = pubnub_subloop_define(pbp, chan, pubnub_subscribe_defopts(), subloop_callback);
    if (NULL == pbsld) {
        printf("Defining a subscribe loop failed\n");
        pubnub_free(pbp);
        return -1;
    }
//! [Define subscribe loop]

    printf("Entering subscribe loop for channel '%s' for %d minutes...\n", chan, minutes_in_loop);

//! [Start a subscribe loop]
    pubnub_subloop_start(pbsld);
//! [Start Subscribe loop]

    just_wait_minutes(minutes_in_loop);

//! [Stop a subscribe loop]
    pubnub_subloop_stop(pbsld);
//! [Start Subscribe loop]

//! [Release a subscribe loop]
    pubnub_subloop_undef(pbsld);
//! [Release Subscribe loop]

    /* We're done */
    if (0 != pubnub_free_with_timeout(pbp, 1000)) {
        puts("Failed to free the context in due time");
    }

    puts("Pubnub callback subloop demo over.");

    return 0;
}
