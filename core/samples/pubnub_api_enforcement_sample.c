/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "core/pubnub_ntf_enforcement.h"
#include "pubnub_api_types.h"
#include "pubnub_sync.h"

#include "core/pubnub_coreapi_ex.h"
#include "core/pubnub_coreapi.h"
#include "core/pubnub_helper.h"
#include "core/pubnub_timers.h"
#include "core/pubnub_subscribe_event_listener.h"
#include "core/pubnub_subscribe_event_engine.h"
#include "core/pubnub_entities.h"
#include "core/pubnub_free_with_timeout.h"

#include <stdio.h>
#include <string.h>
#include <time.h>


/** Helper functions */

/** Waits for the specified number of seconds */
static void wait_seconds(double time_in_seconds);

/** Frees the resources of the sync context */
static void sync_sample_free(pubnub_t* p);

/** Frees the resources of the callback context */
static void callback_sample_free(pubnub_t* p);

/** Callback for message listener */
static void subscribe_message_listener(
    const pubnub_t*                pb,
    const struct pubnub_v2_message message,
    void*                          user_data);

int main()
{
    char const*     user_id = "my_user_id";
    char const*     channel_id = "my_channel";

    pubnub_t*       pbp_sync  = pubnub_alloc();
    pubnub_t*       pbp_callback  = pubnub_alloc();

    printf("PubNub API enforcement demo\n");

    if (NULL == pbp_sync || NULL == pbp_callback) {
        printf("Failed to allocate Pubnub context!\n");
        return -1;
    }

    pubnub_enforce_api(pbp_sync, PNA_SYNC);
    pubnub_enforce_api(pbp_callback, PNA_CALLBACK);

    char* my_env_publish_key = getenv("PUBNUB_PUBLISH_KEY");
    char* my_env_subscribe_key = getenv("PUBNUB_SUBSCRIBE_KEY");

    if (NULL == my_env_publish_key) { my_env_publish_key = "demo"; }
    if (NULL == my_env_subscribe_key) { my_env_subscribe_key = "demo"; }

    pubnub_init(pbp_sync, my_env_publish_key, my_env_subscribe_key);
    pubnub_init(pbp_callback, my_env_publish_key, my_env_subscribe_key);

    pubnub_set_user_id(pbp_sync, user_id);
    pubnub_set_user_id(pbp_callback, user_id);

    pubnub_set_blocking_io(pbp_sync);
    pubnub_set_non_blocking_io(pbp_callback);

    /** Callback context */

    pubnub_channel_t* channel = pubnub_channel_alloc(pbp_callback, channel_id);
    pubnub_subscription_t* subscription =
        pubnub_subscription_alloc((pubnub_entity_t*)channel, NULL);
    pubnub_entity_free((void**)&channel);
    pubnub_subscribe_add_subscription_listener(subscription,
                                               PBSL_LISTENER_ON_MESSAGE,
                                               subscribe_message_listener,
                                               NULL);
    printf("Subscribing with subscription...\n");
    enum pubnub_res rslt = pubnub_subscribe_with_subscription(
        subscription,
        NULL);
    printf("Subscribe with subscription result: %s\n",
           pubnub_res_2_string(rslt));

    /** Sync context */

    /** Wait for the subscription to be established */
    wait_seconds(2);

    printf("Publishing message from sync context...\n");
    enum pubnub_res publish_result = pubnub_publish(pbp_sync, channel_id, "\"Hello world from sync!\"");
    if (PNR_OK != publish_result && PNR_STARTED != publish_result) {
        printf("Failed to publish message from sync context!\n");

        sync_sample_free(pbp_sync);
        pubnub_subscription_free(&subscription);
        callback_sample_free(pbp_callback);
        return -1;
    }

    if (PNR_OK != pubnub_await(pbp_sync)) {
        printf("Failed to await message from sync context!\n");

        sync_sample_free(pbp_sync);
        pubnub_subscription_free(&subscription);
        callback_sample_free(pbp_callback);
        return -1;
    }
    printf("Message published from sync context!\n");

    wait_seconds(2);

    /** Cleanup */

    printf("Unsubscribing from all...\n");
    rslt = pubnub_unsubscribe_all(pbp_callback);
    printf("Unsubscribe from all result: %s\n",
           pubnub_res_2_string(rslt));

    sync_sample_free(pbp_sync);

    pubnub_subscription_free(&subscription);
    callback_sample_free(pbp_callback);

    puts("PubNub api enforcement demo over.");

    return 0;
}

static void sync_sample_free(pubnub_t* p)
{
    if (PN_CANCEL_STARTED == pubnub_cancel(p)) {
        enum pubnub_res pnru = pubnub_await(p);
        if (pnru != PNR_CANCELLED) {
            printf("Awaiting cancel failed: %d('%s')\n",
                   pnru,
                   pubnub_res_2_string(pnru));
        }
    }
    if (pubnub_free(p) != 0) {
        printf("Failed to free the Pubnub context\n");
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

static void subscribe_message_listener(
    const pubnub_t*                pb,
    const struct pubnub_v2_message message,
    void*                          user_data) {
    printf("CALLBACK | Received message: %.*s\n", (int) message.payload.size, message.payload.ptr);
}



