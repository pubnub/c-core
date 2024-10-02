/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_callback.h"
#include "core/pubnub_subscribe_event_engine.h"

#include <stdio.h>

int main()
{
    const char* publish_key = getenv("PUBNUB_PUBLISH_KEY");
    if (NULL == publish_key) { publish_key = "demo"; }
    const char* subscribe_key = getenv("PUBNUB_SUBSCRIBE_KEY");
    if (NULL == subscribe_key) { subscribe_key = "demo"; }

    pubnub_t* pubnub = pubnub_alloc();
    pubnub_init(pubnub, publish_key, subscribe_key);
    pubnub_set_user_id(pubnub, "demo");

    pubnub_channel_t*      channel = pubnub_channel_alloc(pubnub, "my_channel");
    pubnub_subscription_t* subscription =
        pubnub_subscription_alloc((pubnub_entity_t*)channel, NULL);

    pubnub_subscription_free(&subscription);
    pubnub_entity_free((void**)&channel);

    puts("Pubnub subscribe event engine demo is over.");

    return 0;
}