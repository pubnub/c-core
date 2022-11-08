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
        printf("Subscribe failed with code: %d('%s')\n", result, pubnub_res_2_string(result));
    }
}


static void wait_seconds(double time_in_seconds)
{
    time_t  start = time(NULL);
    double time_passed_in_seconds;
    do {
        time_passed_in_seconds = difftime(time(NULL), start);
    } while (time_passed_in_seconds < time_in_seconds);
}


static void callback_sample_free(pubnub_t* p)
{
    if (pubnub_free_with_timeout(p, 1000) != 0) {
        printf("Failed to free the Pubnub context\n");
    }
    else {
        /* Waits until the context is released from the processing queue */
        wait_seconds(2);
    }
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
    pubnub_set_user_id(pbp, "demo");

    //! [Define subscribe loop]
    pbsld = pubnub_subloop_define(
        pbp, chan, pubnub_subscribe_defopts(), subloop_callback);
    if (NULL == pbsld) {
        printf("Defining a subscribe loop failed\n");
        callback_sample_free(pbp);
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

    callback_sample_free(pbp);
    puts("Pubnub callback subloop demo over.");

    return 0;
}
