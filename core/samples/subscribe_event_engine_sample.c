/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#include "pubnub_callback.h"
#include "core/pubnub_subscribe_event_engine.h"
#include "core/pubnub_helper.h"

#include <stdio.h>
#include <time.h>
#include <unistd.h>

static void wait_seconds(double time_in_seconds)
{
    time_t start = time(NULL);
    double time_passed_in_seconds;
    do {
        time_passed_in_seconds = difftime(time(NULL), start);
        usleep(10);
    } while (time_passed_in_seconds < time_in_seconds);
}

int main()
{
    const unsigned minutes_in_loop = 1;
    const char*    publish_key     = getenv("PUBNUB_PUBLISH_KEY");
    if (NULL == publish_key) { publish_key = "demo"; }
    const char* subscribe_key = getenv("PUBNUB_SUBSCRIBE_KEY");
    if (NULL == subscribe_key) { subscribe_key = "demo"; }

    pubnub_t* pubnub = pubnub_alloc();
    pubnub_init(pubnub, publish_key, subscribe_key);
    pubnub_set_user_id(pubnub, "demo");

    pubnub_channel_t*      channel = pubnub_channel_alloc(pubnub, "my_channel");
    pubnub_subscription_t* subscription =
        pubnub_subscription_alloc((pubnub_entity_t*)channel, NULL);

    const enum pubnub_res rslt = pubnub_subscribe_with_subscription(subscription, NULL);
    printf("~~~~> Subscribe result: %s\n", pubnub_res_2_string(rslt));

    // wait_seconds(minutes_in_loop * 60);
    wait_seconds(minutes_in_loop * 5);
    printf("~~~~> 9\n");

    pubnub_unsubscribe_with_subscription(&subscription);
    printf("~~~~> 10\n");
    pubnub_subscription_free(&subscription);
    pubnub_entity_free((void**)&channel);
    printf("~~~~> 11\n");

    puts("Pubnub subscribe event engine demo is over.");

    return 0;
}