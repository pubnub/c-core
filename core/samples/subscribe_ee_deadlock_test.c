/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

/**
 * @file subscribe_ee_deadlock_test.c
 * @brief Deadlock regression test for subscribe event engine.
 *
 * Tests that unsubscribe operations do not deadlock due to AB-BA mutex
 * ordering between ee->mutw and pb->monitor.
 *
 * The IO callback thread acquires pb->monitor then ee->mutw, while user
 * thread functions that hold ee->mutw and call PubNub APIs acquire
 * pb->monitor second — creating a classic AB-BA deadlock.
 *
 * This test exercises three paths:
 *   1. pubnub_unsubscribe_with_subscription
 *   2. pubnub_unsubscribe_with_subscription_set
 *   3. pubnub_unsubscribe_all
 *
 * A watchdog thread kills the process if any operation hangs for more
 * than DEADLOCK_TIMEOUT_SEC seconds.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#if !defined _WIN32
#include <unistd.h>
#include <pthread.h>
#else
#include <windows.h>
#endif

#include "core/pubnub_subscribe_event_engine.h"
#include "core/pubnub_subscribe_event_listener.h"
#include "core/pubnub_ntf_enforcement.h"
#include "core/pubnub_free_with_timeout.h"
#include "core/pubnub_blocking_io.h"
#include "core/pubnub_helper.h"
#include "core/pubnub_alloc.h"
#include "core/pubnub_ntf_sync.h"
#include "pubnub_callback.h"


// ----------------------------------------------
//                  Constants
// ----------------------------------------------

/** Seconds before watchdog declares a deadlock. */
#define DEADLOCK_TIMEOUT_SEC 30

/** Seconds to wait for subscription loop to become active. */
#define SUBSCRIBE_SETTLE_SEC 3

/** Number of test iterations per path to increase deadlock probability. */
#define ITERATIONS_PER_PATH 3


// ----------------------------------------------
//                    Globals
// ----------------------------------------------

static volatile int g_msg_received = 0;
static char g_last_channel[256];
static char g_last_message[256];


// ----------------------------------------------
//              Watchdog thread
// ----------------------------------------------

#if !defined _WIN32
static void* watchdog_thread(void* arg)
{
    (void)arg;
    time_t start = time(NULL);
    while (difftime(time(NULL), start) < DEADLOCK_TIMEOUT_SEC) {
        usleep(100000);
    }
    fprintf(stderr,
            "\n*** DEADLOCK DETECTED: test hung for %d seconds ***\n",
            DEADLOCK_TIMEOUT_SEC);
    fflush(stderr);
    _exit(99);
    return NULL;
}

static pthread_t g_watchdog;

static void watchdog_start(void)
{
    pthread_create(&g_watchdog, NULL, watchdog_thread, NULL);
    pthread_detach(g_watchdog);
}

static void watchdog_reset(void)
{
    pthread_cancel(g_watchdog);
    watchdog_start();
}

static void watchdog_stop(void)
{
    pthread_cancel(g_watchdog);
}
#else
static HANDLE g_watchdog;
static DWORD WINAPI watchdog_thread(LPVOID arg)
{
    (void)arg;
    Sleep(DEADLOCK_TIMEOUT_SEC * 1000);
    fprintf(stderr,
            "\n*** DEADLOCK DETECTED: test hung for %d seconds ***\n",
            DEADLOCK_TIMEOUT_SEC);
    fflush(stderr);
    ExitProcess(99);
    return 0;
}

static void watchdog_start(void)
{
    g_watchdog = CreateThread(NULL, 0, watchdog_thread, NULL, 0, NULL);
}

static void watchdog_reset(void)
{
    TerminateThread(g_watchdog, 0);
    CloseHandle(g_watchdog);
    watchdog_start();
}

static void watchdog_stop(void)
{
    TerminateThread(g_watchdog, 0);
    CloseHandle(g_watchdog);
}
#endif


// ----------------------------------------------
//                  Helpers
// ----------------------------------------------

static void wait_seconds(double time_in_seconds)
{
    time_t start = time(NULL);
    while (difftime(time(NULL), start) < time_in_seconds) {
#if !defined _WIN32
        usleep(10000);
#else
        Sleep(10);
#endif
    }
}

static void message_listener(
    const pubnub_t*                pb,
    const struct pubnub_v2_message message,
    void*                          user_data)
{
    (void)pb;
    (void)user_data;

    snprintf(g_last_channel, sizeof(g_last_channel), "%.*s",
             (int)message.channel.size, message.channel.ptr);
    snprintf(g_last_message, sizeof(g_last_message), "%.*s",
             (int)message.payload.size, message.payload.ptr);

    printf("  [msg] on '%s': %s\n", g_last_channel, g_last_message);
    g_msg_received++;
}

static void status_listener(
    const pubnub_t*                         pb,
    const pubnub_subscription_status        status,
    const pubnub_subscription_status_data_t status_data,
    void*                                   data)
{
    (void)pb;
    (void)data;

    switch (status) {
    case PNSS_SUBSCRIPTION_STATUS_CONNECTED:
        printf("  [status] Connected\n");
        break;
    case PNSS_SUBSCRIPTION_STATUS_CONNECTION_ERROR:
        printf("  [status] Connection error: %s\n",
               pubnub_res_2_string(status_data.reason));
        break;
    case PNSS_SUBSCRIPTION_STATUS_DISCONNECTED_UNEXPECTEDLY:
        printf("  [status] Disconnected unexpectedly: %s\n",
               pubnub_res_2_string(status_data.reason));
        break;
    case PNSS_SUBSCRIPTION_STATUS_DISCONNECTED:
        printf("  [status] Disconnected\n");
        break;
    case PNSS_SUBSCRIPTION_STATUS_SUBSCRIPTION_CHANGED:
        printf("  [status] Subscription changed\n");
        break;
    }
}

static int publish_and_wait(
    pubnub_t*   pb_sync,
    const char* channel,
    const char* msg)
{
    enum pubnub_res res = pubnub_publish(pb_sync, channel, msg);
    if (PNR_OK != res && PNR_STARTED != res) {
        printf("  Publish initiation failed: %s\n", pubnub_res_2_string(res));
        return -1;
    }
    res = pubnub_await(pb_sync);
    if (PNR_OK != res) {
        printf("  Publish await failed: %s\n", pubnub_res_2_string(res));
        return -1;
    }
    return 0;
}

static int wait_for_message(int expected_count, double timeout_sec)
{
    time_t start = time(NULL);
    while (g_msg_received < expected_count) {
        if (difftime(time(NULL), start) >= timeout_sec) {
            printf("  Timeout waiting for message (got %d, want %d)\n",
                   g_msg_received, expected_count);
            return -1;
        }
#if !defined _WIN32
        usleep(10000);
#else
        Sleep(10);
#endif
    }
    return 0;
}


// ----------------------------------------------
//                  Test paths
// ----------------------------------------------

/**
 * Test 1: Subscribe two channels via individual subscriptions,
 *         then unsubscribe one while IO thread is active.
 */
static int test_unsubscribe_with_subscription(
    pubnub_t* pb_cb,
    pubnub_t* pb_sync)
{
    printf("\n=== Test 1: pubnub_unsubscribe_with_subscription ===\n");

    pubnub_channel_t* ch1 = pubnub_channel_alloc(pb_cb, "deadlock-test-ch1");
    pubnub_channel_t* ch2 = pubnub_channel_alloc(pb_cb, "deadlock-test-ch2");
    pubnub_subscription_t* sub1 =
        pubnub_subscription_alloc((pubnub_entity_t*)ch1, NULL);
    pubnub_subscription_t* sub2 =
        pubnub_subscription_alloc((pubnub_entity_t*)ch2, NULL);
    pubnub_entity_free((void**)&ch1);
    pubnub_entity_free((void**)&ch2);

    pubnub_subscribe_add_subscription_listener(
        sub1, PBSL_LISTENER_ON_MESSAGE, message_listener, NULL);
    pubnub_subscribe_add_subscription_listener(
        sub2, PBSL_LISTENER_ON_MESSAGE, message_listener, NULL);

    enum pubnub_res res;
    res = pubnub_subscribe_with_subscription(sub1, NULL);
    printf("  Subscribe ch1: %s\n", pubnub_res_2_string(res));
    if (PNR_OK != res) { return -1; }

    res = pubnub_subscribe_with_subscription(sub2, NULL);
    printf("  Subscribe ch2: %s\n", pubnub_res_2_string(res));
    if (PNR_OK != res) { return -1; }

    wait_seconds(SUBSCRIBE_SETTLE_SEC);

    /* Publish to verify subscription is active. */
    int msg_before = g_msg_received;
    if (0 == publish_and_wait(pb_sync, "deadlock-test-ch1", "\"ping1\"")) {
        wait_for_message(msg_before + 1, 5);
    }

    /*
     * THE CRITICAL TEST: unsubscribe from ch2 while IO thread is active.
     * This is where the AB-BA deadlock occurred before the fix.
     */
    printf("  Unsubscribing from ch2 (deadlock-prone path)...\n");
    watchdog_reset();
    res = pubnub_unsubscribe_with_subscription(&sub2);
    printf("  Unsubscribe ch2: %s\n", pubnub_res_2_string(res));

    pubnub_subscription_free(&sub2);
    wait_seconds(1);

    /* Verify ch1 still works after partial unsubscribe. */
    msg_before = g_msg_received;
    if (0 == publish_and_wait(pb_sync, "deadlock-test-ch1", "\"after-unsub\"")) {
        if (0 != wait_for_message(msg_before + 1, 10)) {
            printf("  WARNING: ch1 message not received after unsub from ch2\n");
        }
    }

    /* Cleanup: unsubscribe all remaining. */
    pubnub_unsubscribe_all(pb_cb);
    wait_seconds(1);
    pubnub_subscription_free(&sub1);

    printf("=== Test 1 PASSED (no deadlock) ===\n");
    return 0;
}

/**
 * Test 2: Subscribe via subscription set, then unsubscribe the set
 *         while IO thread is active.
 */
static int test_unsubscribe_with_subscription_set(
    pubnub_t* pb_cb,
    pubnub_t* pb_sync)
{
    printf("\n=== Test 2: pubnub_unsubscribe_with_subscription_set ===\n");

    pubnub_channel_t* ch1 = pubnub_channel_alloc(pb_cb, "deadlock-test-set-ch1");
    pubnub_channel_t* ch2 = pubnub_channel_alloc(pb_cb, "deadlock-test-set-ch2");
    pubnub_entity_t** entities = malloc(2 * sizeof(pubnub_entity_t*));
    entities[0] = (pubnub_entity_t*)ch1;
    entities[1] = (pubnub_entity_t*)ch2;

    pubnub_subscription_set_t* set =
        pubnub_subscription_set_alloc_with_entities(entities, 2, NULL);
    pubnub_entity_free((void**)&ch1);
    pubnub_entity_free((void**)&ch2);
    free(entities);

    pubnub_subscribe_add_subscription_set_listener(
        set, PBSL_LISTENER_ON_MESSAGE, message_listener, NULL);

    enum pubnub_res res;
    res = pubnub_subscribe_with_subscription_set(set, NULL);
    printf("  Subscribe set: %s\n", pubnub_res_2_string(res));
    if (PNR_OK != res) { return -1; }

    wait_seconds(SUBSCRIBE_SETTLE_SEC);

    /* Publish to verify subscription is active. */
    int msg_before = g_msg_received;
    if (0 == publish_and_wait(pb_sync, "deadlock-test-set-ch1", "\"set-ping\"")) {
        wait_for_message(msg_before + 1, 5);
    }

    /*
     * THE CRITICAL TEST: unsubscribe set while IO thread is active.
     */
    printf("  Unsubscribing set (deadlock-prone path)...\n");
    watchdog_reset();
    res = pubnub_unsubscribe_with_subscription_set(&set);
    printf("  Unsubscribe set: %s\n", pubnub_res_2_string(res));

    wait_seconds(1);
    if (NULL != set) { pubnub_subscription_set_free(&set); }

    printf("=== Test 2 PASSED (no deadlock) ===\n");
    return 0;
}

/**
 * Test 3: Subscribe to channels, then call pubnub_unsubscribe_all
 *         while IO thread is active.
 */
static int test_unsubscribe_all(
    pubnub_t* pb_cb,
    pubnub_t* pb_sync)
{
    printf("\n=== Test 3: pubnub_unsubscribe_all ===\n");

    pubnub_channel_t* ch1 = pubnub_channel_alloc(pb_cb, "deadlock-test-all-ch1");
    pubnub_channel_t* ch2 = pubnub_channel_alloc(pb_cb, "deadlock-test-all-ch2");
    pubnub_subscription_t* sub1 =
        pubnub_subscription_alloc((pubnub_entity_t*)ch1, NULL);
    pubnub_subscription_t* sub2 =
        pubnub_subscription_alloc((pubnub_entity_t*)ch2, NULL);
    pubnub_entity_free((void**)&ch1);
    pubnub_entity_free((void**)&ch2);

    pubnub_subscribe_add_subscription_listener(
        sub1, PBSL_LISTENER_ON_MESSAGE, message_listener, NULL);
    pubnub_subscribe_add_subscription_listener(
        sub2, PBSL_LISTENER_ON_MESSAGE, message_listener, NULL);

    enum pubnub_res res;
    res = pubnub_subscribe_with_subscription(sub1, NULL);
    printf("  Subscribe ch1: %s\n", pubnub_res_2_string(res));
    if (PNR_OK != res) { return -1; }

    res = pubnub_subscribe_with_subscription(sub2, NULL);
    printf("  Subscribe ch2: %s\n", pubnub_res_2_string(res));
    if (PNR_OK != res) { return -1; }

    wait_seconds(SUBSCRIBE_SETTLE_SEC);

    /* Publish to verify subscription is active. */
    int msg_before = g_msg_received;
    if (0 == publish_and_wait(pb_sync, "deadlock-test-all-ch1", "\"all-ping\"")) {
        wait_for_message(msg_before + 1, 5);
    }

    /*
     * THE CRITICAL TEST: unsubscribe_all while IO thread is active.
     */
    printf("  Calling pubnub_unsubscribe_all (deadlock-prone path)...\n");
    watchdog_reset();
    res = pubnub_unsubscribe_all(pb_cb);
    printf("  Unsubscribe all: %s\n", pubnub_res_2_string(res));

    wait_seconds(1);
    pubnub_subscription_free(&sub1);
    pubnub_subscription_free(&sub2);

    printf("=== Test 3 PASSED (no deadlock) ===\n");
    return 0;
}

/**
 * Test 4: The exact user reproduction case — subscribe two individual
 *         subscriptions, publish/verify, then unsubscribe from the
 *         second while the first is still receiving.
 */
static int test_user_reproduction_case(
    pubnub_t* pb_cb,
    pubnub_t* pb_sync)
{
    printf("\n=== Test 4: User reproduction case ===\n");

    const char* channel1 = "deadlock-repro-ch1";
    const char* channel2 = "deadlock-repro-ch2";

    /* Step 1: Subscribe to channel1 and verify. */
    printf("  Step 1: Subscribe to channel1\n");
    pubnub_channel_t* ch1 = pubnub_channel_alloc(pb_cb, channel1);
    pubnub_subscription_t* sub1 =
        pubnub_subscription_alloc((pubnub_entity_t*)ch1, NULL);
    pubnub_entity_free((void**)&ch1);
    pubnub_subscribe_add_subscription_listener(
        sub1, PBSL_LISTENER_ON_MESSAGE, message_listener, NULL);

    enum pubnub_res res = pubnub_subscribe_with_subscription(sub1, NULL);
    printf("  Subscribe result: %s\n", pubnub_res_2_string(res));
    wait_seconds(2);

    int msg_before = g_msg_received;
    if (0 == publish_and_wait(pb_sync, channel1, "\"msg1\"")) {
        if (0 != wait_for_message(msg_before + 1, 5)) {
            printf("  WARNING: msg1 not received\n");
        }
    }
    printf("  Step 1 OK\n");

    /* Step 2: Subscribe to channel2 and verify. */
    printf("  Step 2: Subscribe to channel2\n");
    pubnub_channel_t* ch2 = pubnub_channel_alloc(pb_cb, channel2);
    pubnub_subscription_t* sub2 =
        pubnub_subscription_alloc((pubnub_entity_t*)ch2, NULL);
    pubnub_entity_free((void**)&ch2);
    pubnub_subscribe_add_subscription_listener(
        sub2, PBSL_LISTENER_ON_MESSAGE, message_listener, NULL);

    res = pubnub_subscribe_with_subscription(sub2, NULL);
    printf("  Subscribe result: %s\n", pubnub_res_2_string(res));
    wait_seconds(2);

    msg_before = g_msg_received;
    if (0 == publish_and_wait(pb_sync, channel2, "\"msg2\"")) {
        if (0 != wait_for_message(msg_before + 1, 5)) {
            printf("  WARNING: msg2 not received\n");
        }
    }
    printf("  Step 2 OK\n");

    /*
     * Step 3: THE DEADLOCK POINT — unsubscribe from channel2, then
     * verify channel1 still works.
     */
    printf("  Step 3: Unsubscribe from channel2 (DEADLOCK POINT)\n");
    watchdog_reset();
    res = pubnub_unsubscribe_with_subscription(&sub2);
    printf("  Unsubscribe result: %s\n", pubnub_res_2_string(res));

    pubnub_subscription_free(&sub2);
    printf("  Waiting for subscribe loop to restart...\n");
    wait_seconds(3);

    printf("  Publishing to channel1 to verify it still works...\n");
    msg_before = g_msg_received;
    if (0 == publish_and_wait(pb_sync, channel1, "\"msg3\"")) {
        if (0 != wait_for_message(msg_before + 1, 10)) {
            printf("  WARNING: msg3 not received on ch1 after ch2 unsub\n");
        }
    }
    printf("  Step 3 OK\n");

    /* Cleanup. */
    pubnub_unsubscribe_all(pb_cb);
    wait_seconds(1);
    pubnub_subscription_free(&sub1);

    printf("=== Test 4 PASSED (no deadlock) ===\n");
    return 0;
}


// ----------------------------------------------
//                    Main
// ----------------------------------------------

int main(void)
{
    setbuf(stdout, NULL);
    printf("Subscribe Event Engine Deadlock Regression Test\n");
    printf("Watchdog timeout: %d seconds per operation\n\n",
           DEADLOCK_TIMEOUT_SEC);

    char* pub_key = getenv("PUBNUB_PUBLISH_KEY");
    char* sub_key = getenv("PUBNUB_SUBSCRIBE_KEY");
    if (NULL == pub_key) { pub_key = "demo"; }
    if (NULL == sub_key) { sub_key = "demo"; }

    pubnub_t* pb_sync = pubnub_alloc();
    pubnub_t* pb_cb   = pubnub_alloc();
    if (NULL == pb_sync || NULL == pb_cb) {
        fprintf(stderr, "Failed to allocate PubNub contexts\n");
        return 1;
    }

    pubnub_enforce_api(pb_sync, PNA_SYNC);
    pubnub_enforce_api(pb_cb,   PNA_CALLBACK);
    pubnub_init(pb_sync, pub_key, sub_key);
    pubnub_init(pb_cb,   pub_key, sub_key);
    pubnub_set_user_id(pb_sync, "deadlock-test-sync");
    pubnub_set_user_id(pb_cb,   "deadlock-test-cb");

    pubnub_subscribe_add_status_listener(pb_cb, status_listener, NULL);

    int result = 0;

    watchdog_start();

    /* Test 4 first — this is the exact user reproduction case. */
    if (0 != test_user_reproduction_case(pb_cb, pb_sync)) {
        printf("Test 4 FAILED\n");
        result = 1;
    }

    if (0 == result) {
        watchdog_reset();
        if (0 != test_unsubscribe_with_subscription(pb_cb, pb_sync)) {
            printf("Test 1 FAILED\n");
            result = 1;
        }
    }

    if (0 == result) {
        watchdog_reset();
        if (0 != test_unsubscribe_with_subscription_set(pb_cb, pb_sync)) {
            printf("Test 2 FAILED\n");
            result = 1;
        }
    }

    if (0 == result) {
        watchdog_reset();
        if (0 != test_unsubscribe_all(pb_cb, pb_sync)) {
            printf("Test 3 FAILED\n");
            result = 1;
        }
    }

    if (0 == result) {
        printf("\n*** ALL DEADLOCK TESTS PASSED ***\n");
    }
    else {
        printf("\n*** SOME TESTS FAILED ***\n");
    }

    watchdog_stop();

    /*
     * Context teardown with NTF runtime selection can block indefinitely
     * (long-poll subscribe timeout, internal state cleanup). Since the
     * test verdict is already decided, force-exit to keep CI fast.
     */
    _exit(result);
}
