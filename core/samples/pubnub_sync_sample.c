/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_posix_sync.h"

#include <stdio.h>


int main()
{
    char const *msg;
    enum pubnub_res res;
    char const *chan = "hello_world";
    pubnub_t *pbp = pubnub_alloc();
    struct Pubnub_UUID uuid;
    struct Pubnub_UUID_String str_uuid;

    if (NULL == pbp) {
        printf("Failed to allocate Pubnub context!\n");
        return -1;
    }
    pubnub_init(pbp, "demo", "demo");

//    pubnub_set_non_blocking_io(pbp);
    
    if (0 != pubnub_generate_uuid_v4_random(&uuid)) {
        pubnub_set_uuid(pbp, "zeka-peka-iz-jendeka");
    }
    else {
        str_uuid = pubnub_uuid_to_string(&uuid);
        pubnub_set_uuid(pbp, str_uuid.uuid);
        printf("Generated UUID: %s\n", str_uuid.uuid);
    }

    pubnub_set_auth(pbp, "danaske");

    puts("Publishing...");
    res = pubnub_publish(pbp, chan, "\"Hello world from sync!\"");
    if (res != PNR_STARTED) {
        printf("pubnub_publish() returned unexpected: %d\n", res);
        pubnub_free(pbp);
        return -1;
    }
    res = pubnub_await(pbp);
    if (res == PNR_STARTED) {
        printf("pubnub_await() returned unexpected: PNR_STARTED(%d)\n", res);
        pubnub_free(pbp);
        return -1;
    }
    if (PNR_OK == res) {
        printf("Published! Response from Pubnub: %s\n", pubnub_last_publish_result(pbp));
    }
    else if (PNR_PUBLISH_FAILED == res) {
        printf("Published failed on Pubnub, description: %s\n", pubnub_last_publish_result(pbp));
    }
    else {
        printf("Publishing failed with code: %d\n", res);
    }

    puts("Subscribing...");
    res = pubnub_subscribe(pbp, chan, NULL);
    if (res != PNR_STARTED) {
        printf("pubnub_subscribe() returned unexpected: %d\n", res);
        pubnub_free(pbp);
        return -1;
    }
    res = pubnub_await(pbp);
    if (res == PNR_STARTED) {
        printf("pubnub_await() returned unexpected: PNR_STARTED(%d)\n", res);
        pubnub_free(pbp);
        return -1;
    }

    if (PNR_OK == res) {
        puts("Subscribed!");
    }
    else {
        printf("Subscribing failed with code: %d\n", res);
    }

    res = pubnub_subscribe(pbp, chan, NULL);
    if (res != PNR_STARTED) {
        printf("pubnub_subscribe() returned unexpected: %d\n", res);
        pubnub_free(pbp);
        return -1;
    }
    res = pubnub_await(pbp);
    if (res == PNR_STARTED) {
        printf("pubnub_await() returned unexpected: PNR_STARTED(%d)\n", res);
        pubnub_free(pbp);
        return -1;
    }

    if (PNR_OK == res) {
        puts("Subscribed! Got messages:");
        for (;;) {
            msg = pubnub_get(pbp);
            if (NULL == msg) {
                break;
            }
            puts(msg);
        }
    }
    else {
        printf("Subscribing failed with code: %d\n", res);
    }

    puts("Getting time...");
    res = pubnub_time(pbp);
    if (res != PNR_STARTED) {
        printf("pubnub_time() returned unexpected: %d\n", res);
        pubnub_free(pbp);
        return -1;
    }
    res = pubnub_await(pbp);
    if (res == PNR_STARTED) {
        printf("pubnub_await() returned unexpected: PNR_STARTED(%d)\n", res);
        pubnub_free(pbp);
        return -1;
    }

    if (PNR_OK == res) {
        printf("Gotten time: %s; last time token=%s\n", pubnub_get(pbp), pubnub_last_time_token(pbp));
    }
    else {
        printf("Getting time failed with code: %d\n", res);
    }


    puts("Getting history...");
    res = pubnub_history(pbp, chan, NULL, 10);
    if (res != PNR_STARTED) {
        printf("pubnub_history() returned unexpected: %d\n", res);
        pubnub_free(pbp);
        return -1;
    }
    res = pubnub_await(pbp);
    if (res == PNR_STARTED) {
        printf("pubnub_await() returned unexpected: PNR_STARTED(%d)\n", res);
        pubnub_free(pbp);
        return -1;
    }

    if (PNR_OK == res) {
        puts("Got history! Messages:");
        for (;;) {
            msg = pubnub_get(pbp);
            if (NULL == msg) {
                break;
            }
            puts(msg);
        }
    }
    else {
        printf("Getting history failed with code: %d\n", res);
    }

    puts("Getting historyv2 with include_token...");
    res = pubnub_historyv2(pbp, chan, NULL, 10, true);
    if (res != PNR_STARTED) {
        printf("pubnub_historyv2() returned unexpected: %d\n", res);
        pubnub_free(pbp);
        return -1;
    }
    res = pubnub_await(pbp);
    if (res == PNR_STARTED) {
        printf("pubnub_await() returned unexpected: PNR_STARTED(%d)\n", res);
        pubnub_free(pbp);
        return -1;
    }

    if (PNR_OK == res) {
        puts("Got history v2! Elements:");
        for (;;) {
            msg = pubnub_get(pbp);
            if (NULL == msg) {
                break;
            }
            puts(msg);
        }
    }
    else {
        printf("Getting history v2 with include_token failed with code: %d\n", res);
    }

    puts("Getting here_now presence...");
    res = pubnub_here_now(pbp, chan, NULL);
    if (res != PNR_STARTED) {
        printf("pubnub_here_now() returned unexpected: %d\n", res);
        pubnub_free(pbp);
        return -1;
    }
    res = pubnub_await(pbp);
    if (res == PNR_STARTED) {
        printf("pubnub_await() returned unexpected: PNR_STARTED(%d)\n", res);
        pubnub_free(pbp);
        return -1;
    }

    if (PNR_OK == res) {
        puts("Got here now presence!");
        for (;;) {
            msg = pubnub_get(pbp);
            if (NULL == msg) {
                break;
            }
            puts(msg);
        }
    }
    else {
        printf("Getting here-now presence failed with code: %d\n", res);
    }

/** Global here_now presence for "demo" subscribe key is _very_
    long, so we disable it. Enable to try out, or if you use
    a "real" subscribe key*/
#if 0
    puts("Getting global here_now presence...");
    res = pubnub_global_here_now(pbp);
    if (res != PNR_STARTED) {
        printf("pubnub_global_here_now() returned unexpected: %d\n", res);
        pubnub_free(pbp);
        return -1;
    }
    res = pubnub_await(pbp);
    if (res == PNR_STARTED) {
        printf("pubnub_await() returned unexpected: PNR_STARTED(%d)\n", res);
        pubnub_free(pbp);
        return -1;
    }

    if (PNR_OK == res) {
        puts("Got global here now presence!");
        for (;;) {
            msg = pubnub_get(pbp);
            if (NULL == msg) {
                break;
            }
            puts(msg);
        }
    }
    else {
        printf("Getting global here-now presence failed with code: %d\n", res);
    }
#endif

    puts("Getting where_now presence...");
    res = pubnub_where_now(pbp, pubnub_get_uuid(pbp));
    if (res != PNR_STARTED) {
        printf("pubnub_where_now() returned unexpected: %d\n", res);
        pubnub_free(pbp);
        return -1;
    }
    res = pubnub_await(pbp);
    if (res == PNR_STARTED) {
        printf("pubnub_await() returned unexpected: PNR_STARTED(%d)\n", res);
        pubnub_free(pbp);
        return -1;
    }

    if (PNR_OK == res) {
        puts("Got where now presence!");
        for (;;) {
            msg = pubnub_get(pbp);
            if (NULL == msg) {
                break;
            }
            puts(msg);
        }
    }
    else {
        printf("Getting where-now presence failed with code: %d\n", res);
    }

    puts("Setting state...");
    res = pubnub_set_state(pbp, chan, NULL, pubnub_get_uuid(pbp), "{\"x\":5}");
    if (res != PNR_STARTED) {
        printf("pubnub_set_state() returned unexpected: %d\n", res);
        pubnub_free(pbp);
        return -1;
    }
    res = pubnub_await(pbp);
    if (res == PNR_STARTED) {
        printf("pubnub_await() returned unexpected: PNR_STARTED(%d)\n", res);
        pubnub_free(pbp);
        return -1;
    }

    if (PNR_OK == res) {
        puts("Got set state response!");
        for (;;) {
            msg = pubnub_get(pbp);
            if (NULL == msg) {
                break;
            }
            puts(msg);
        }
    }
    else {
        printf("Setting state failed with code: %d\n", res);
    }

    puts("Getting state...");
    res = pubnub_state_get(pbp, chan, NULL, pubnub_get_uuid(pbp));
    if (res != PNR_STARTED) {
        printf("pubnub_state_get() returned unexpected: %d\n", res);
        pubnub_free(pbp);
        return -1;
    }
    res = pubnub_await(pbp);
    if (res == PNR_STARTED) {
        printf("pubnub_await() returned unexpected: PNR_STARTED(%d)\n", res);
        pubnub_free(pbp);
        return -1;
    }

    if (PNR_OK == res) {
        puts("Got state!");
        for (;;) {
            msg = pubnub_get(pbp);
            if (NULL == msg) {
                break;
            }
            puts(msg);
        }
    }
    else {
        printf("Getting state failed with code: %d\n", res);
    }

    /* We're done */
    if (pubnub_free(pbp) != 0) {
        printf("Failed to free the Pubnub context\n");
    }

    puts("Pubnub POSIX sync demo over.");

    return 0;
}
