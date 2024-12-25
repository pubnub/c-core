/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#include <string.h>
#if !defined _WIN32
#include <unistd.h>
#else
#include <windows.h>
#endif
#include <stdio.h>
#include <time.h>

#include "core/pubnub_subscribe_event_listener.h"
#include "core/pubnub_subscribe_event_engine.h"
#include "core/pubnub_helper.h"
#include "pubnub_callback.h"


// ----------------------------------------------
//              Function prototypes
// ----------------------------------------------

/**
 * @brief Common message listener callback function.
 *
 * @param pb      Pointer to the PubNub context which received real-time update.
 * @param global  Whether listener called from PubNub context or specific
 *                entity.
 * @param message Received real-time update information.
 */
void subscribe_message_listener_(
    const pubnub_t*          pb,
    bool                     global,
    struct pubnub_v2_message message);


// ----------------------------------------------
//                   Functions
// ----------------------------------------------

/**
 * @brief Create string from the memory block.
 *
 * @param block Memory block with information required to create string.
 * @return Pointer to the string or `NULL` in case if empty memory block has
 *         been provided.
 */
static char* string_from_mem_block(struct pubnub_char_mem_block block)
{
    if (0 == block.size) { return NULL; }

    char* string = malloc(block.size + 1);
    memcpy(string, block.ptr, block.size);
    string[block.size] = '\0';

    return string;
}

/**
 * @brief Translate PubNub-defined real-time update type (enum field) to
 *        human-readable format.
 *
 * @param type PubNub-defined real-time update type.
 * @return Human-readable real-time update type.
 */
static char* message_type_2_string(const enum pubnub_message_type type)
{
    switch (type) {
    case pbsbSignal:
        return "signal";
    case pbsbPublished:
        return "message / presence update";
    case pbsbAction:
        return "action";
    case pbsbObjects:
        return "app context";
    case pbsbFiles:
        return "file";
    }

    return "unknown update";
}

/**
 * @brief Temporarily pause app execution.
 *
 * @param time_in_seconds How long further execution should be postponed.
 */
static void wait_seconds(const double time_in_seconds)
{
    const time_t start = time(NULL);
    double       time_passed_in_seconds;
    do {
        time_passed_in_seconds = difftime(time(NULL), start);
#if !defined _WIN32
        usleep(10);
#else
        Sleep(1);
#endif
    } while (time_passed_in_seconds < time_in_seconds);
}

/**
 * @brief Subscription status change listener callback.
 *
 * @param pb          Pointer to the PubNub context for which subscription
 *                    status has been changed.
 * @param status      Current subscription status.
 * @param status_data Information from subscriber.
 */
void subscribe_status_change_listener(
    const pubnub_t*                         pb,
    const pubnub_subscription_status        status,
    const pubnub_subscription_status_data_t status_data)
{
    switch (status) {
    case PNSS_SUBSCRIPTION_STATUS_CONNECTED:
        printf("PubNub client connected to:\n");
        break;
    case PNSS_SUBSCRIPTION_STATUS_CONNECTION_ERROR:
        printf("PubNub client connection error: %s\n",
               pubnub_res_2_string(status_data.reason));
        break;
    case PNSS_SUBSCRIPTION_STATUS_DISCONNECTED_UNEXPECTEDLY:
        printf("PubNub client disconnected unexpectedly with error: %s\n",
               pubnub_res_2_string(status_data.reason));
        break;
    case PNSS_SUBSCRIPTION_STATUS_DISCONNECTED:
        printf("PubNub client disconnected:\n");
        break;
    case PNSS_SUBSCRIPTION_STATUS_SUBSCRIPTION_CHANGED:
        printf("PubNub client changed subscription:\n");
        break;
    }

    if (NULL != status_data.channels)
        printf("\t- channels: %s\n", status_data.channels);
    if (NULL != status_data.channel_groups)
        printf("\t- channel groups: %s\n", status_data.channel_groups);
}

/**
 * @brief Global message listener callback function.
 *
 * @param pb      Pointer to the PubNub context which received real-time update.
 * @param message Received real-time update information.
 */
void global_message_listener(
    const pubnub_t*                pb,
    const struct pubnub_v2_message message)
{
    subscribe_message_listener_(pb, true, message);
}

/**
 * @brief Subscription / subscription set message listener callback function.
 *
 * @param pb      Pointer to the PubNub context which received real-time update.
 * @param message Received real-time update information.
 */
void subscribe_message_listener(
    const pubnub_t*                pb,
    const struct pubnub_v2_message message)
{
    subscribe_message_listener_(pb, false, message);
}

void subscribe_message_listener_(
    const pubnub_t*                pb,
    const bool                     global,
    const struct pubnub_v2_message message)
{
    char* uuid = string_from_mem_block(message.publisher);
    char* ch   = string_from_mem_block(message.channel);
    char* msg  = string_from_mem_block(message.payload);
    char* tt   = string_from_mem_block(message.tt);
    char* type = message_type_2_string(message.message_type);
    char  client[100];

    if (global) {
        snprintf(client, sizeof(client), "PubNub (%p) received", pb);
    }
    else { snprintf(client, sizeof(client), "Received"); }

    printf("%s %s on '%s' at %s:\n", client, type, ch, tt);
    if (NULL != uuid) {
        printf("\t- publisher: %s\n", uuid);
        free(uuid);
    }
    if (NULL != msg) {
        printf("\t- message: %s\n", msg);
        free(msg);
    }

    free(ch);
    free(tt);
}

int main()
{
    /** Setup PubNub context. */
    const char* publish_key = getenv("PUBNUB_PUBLISH_KEY");
    if (NULL == publish_key) { publish_key = "demo"; }
    const char* subscribe_key = getenv("PUBNUB_SUBSCRIBE_KEY");
    if (NULL == subscribe_key) { subscribe_key = "demo"; }

    pubnub_t* pubnub = pubnub_alloc();
    pubnub_init(pubnub, publish_key, subscribe_key);
    pubnub_set_user_id(pubnub, "demo-user");

    /** Add subscription status change listener. */
    pubnub_subscribe_add_status_listener(pubnub,
                                         subscribe_status_change_listener);
    pubnub_subscribe_add_message_listener(pubnub,
                                          PBSL_LISTENER_ON_MESSAGE,
                                          global_message_listener);


    /**
     * Single subscription setup.
     */
    pubnub_channel_t* channel = pubnub_channel_alloc(
        pubnub,
        "channel-test-history1");
    pubnub_subscription_t* subscription =
        pubnub_subscription_alloc((pubnub_entity_t*)channel, NULL);
    /** Subscription retained entity and it is safe to free */
    pubnub_entity_free((void**)&channel);
    /** Add messages listeners for subscription. */
    pubnub_subscribe_add_subscription_listener(subscription,
                                               PBSL_LISTENER_ON_MESSAGE,
                                               subscribe_message_listener);
    printf("Subscribing with subscription...\n");
    /** Subscribe using subscription. */
    enum pubnub_res rslt = pubnub_subscribe_with_subscription(
        subscription,
        NULL);
    printf("Subscribe with subscription result: %s\n",
           pubnub_res_2_string(rslt));


    /**
     * Subscription set setup.
     */
    pubnub_channel_t* channel_mx1 = pubnub_channel_alloc(
        pubnub,
        "channel-test-history1");
    pubnub_channel_t* channel_mx2 = pubnub_channel_alloc(
        pubnub,
        "channel-test-history2");
    pubnub_channel_group_t* group_mx3 = pubnub_channel_group_alloc(
        pubnub,
        "my-channel-group");
    pubnub_entity_t** entities     = malloc(3 * sizeof(pubnub_entity_t*));
    entities[0]                    = (pubnub_entity_t*)channel_mx1;
    entities[1]                    = (pubnub_entity_t*)channel_mx2;
    entities[2]                    = (pubnub_entity_t*)group_mx3;
    pubnub_subscription_set_t* set =
        pubnub_subscription_set_alloc_with_entities(entities, 3, NULL);
    /** Unredlying subscriptions in set retained entities and it is safe to free */
    pubnub_entity_free((void**)&channel_mx1);
    pubnub_entity_free((void**)&channel_mx2);
    pubnub_entity_free((void**)&group_mx3);
    free(entities);

    /** Add messages listeners for subscription set. */
    pubnub_subscribe_add_subscription_set_listener(
        set,
        PBSL_LISTENER_ON_MESSAGE,
        subscribe_message_listener);
    printf("Subscribing with subscription set...\n");
    rslt = pubnub_subscribe_with_subscription_set(
        set,
        NULL);
    printf("Subscribe with subscription set result: %s\n",
           pubnub_res_2_string(rslt));

    /** Wait for messages published to one of the channels (manual). */
    wait_seconds(60);


    /**
     * Remove subscription from subscription set (which is equal to unsubscribe
     * on actively subscribed set).
     */
    size_t                  count = 0;
    pubnub_subscription_t** subs  = pubnub_subscription_set_subscriptions(
        set,
        &count);
    /**
     * Removing first subscription in the list which will be `entities[0]`.
     *
     * Important: Actual unsubscribe won't happen because there exist separate
     * subscription for the same entity for which subscription will be removed
     * from set.
     */
    printf("Removing subscription from subscription set...\n");
    rslt = pubnub_subscription_set_remove(set, &subs[0]);
    printf("Subscription remove result: %s\n",
           pubnub_res_2_string(rslt));
    free(subs);


    /**
     * Unsubscribe using subscription.
     */
    printf("Unsubscribing from subscription...\n");
    rslt = pubnub_unsubscribe_with_subscription(&subscription);
    printf("Unsubscribe with subscription result: %s\n",
           pubnub_res_2_string(rslt));
    pubnub_subscription_free(&subscription);

    /** Sleep a bit before compltion of all stuff. */
    wait_seconds(3);


    /**
     * Unsubscribe from everything.
     */
    printf("Unsubscribing from all...\n");
    rslt = pubnub_unsubscribe_all(pubnub);
    printf("Unsubscribe from all result: %s\n",
           pubnub_res_2_string(rslt));

    /** Giving some time to complete unsubscribe. */
    wait_seconds(3);

    if (NULL != set) { pubnub_subscription_set_free(&set); }

    puts("Pubnub subscribe event engine demo is over.");

    return 0;
}