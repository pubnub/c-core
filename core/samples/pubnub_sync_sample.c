/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_sync.h"

#include "pubnub_helper.h"
#include "pubnub_timers.h"

#include <stdio.h>
#include <time.h>


static void generate_uuid(pubnub_t *pbp)
{
    char const *uuid_default = "zeka-peka-iz-jendeka";
    struct Pubnub_UUID uuid;
    static struct Pubnub_UUID_String str_uuid;

    if (0 != pubnub_generate_uuid_v4_random(&uuid)) {
        pubnub_set_uuid(pbp, uuid_default);
    }
    else {
        str_uuid = pubnub_uuid_to_string(&uuid);
        pubnub_set_uuid(pbp, str_uuid.uuid);
        printf("Generated UUID: %s\n", str_uuid.uuid);
    }
}


static int do_time(pubnub_t *pbp)
{
    enum pubnub_res res;

    puts("-----------------------");
    puts("Getting time...");
    puts("-----------------------");
    res = pubnub_time(pbp);
    if (res != PNR_STARTED) {
        printf("pubnub_time() returned unexpected %d: %s\n", res, pubnub_res_2_string(res));
        pubnub_free(pbp);
        return -1;
    }
    res = pubnub_await(pbp);
    if (res == PNR_STARTED) {
        printf("await() returned unexpected: PNR_STARTED(%d)\n", res);
        pubnub_free(pbp);
        return -1;
    }

    if (PNR_OK == res) {
        printf("Gotten time: %s; last time token=%s\n", pubnub_get(pbp), pubnub_last_time_token(pbp));
    }
    else {
        printf("Getting time failed with code %d: %s\n", res, pubnub_res_2_string(res));
    }

    return 0;
}


int main()
{
    clock_t clk;
    char const *msg;
    enum pubnub_res res;
    char const *chan = "hello_world";
    pubnub_t *pbp = pubnub_alloc();

    if (NULL == pbp) {
        printf("Failed to allocate Pubnub context!\n");
        return -1;
    }
    pubnub_init(pbp, "demo", "demo");

    pubnub_set_transaction_timeout(pbp, PUBNUB_DEFAULT_NON_SUBSCRIBE_TIMEOUT);
    
    /* Leave this commented out to use the default - which is
       blocking I/O on most platforms. Uncomment to use non-
       blocking I/O.
    */
    pubnub_set_non_blocking_io(pbp);

    generate_uuid(pbp);

    pubnub_set_auth(pbp, "danaske");

    puts("Publishing...");
    clk = clock();
    res = pubnub_publish(pbp, chan, "\"Hello world from sync!\"");
    if (res != PNR_STARTED) {
        printf("pubnub_publish() returned unexpected: %d\n", res);
        pubnub_free(pbp);
        return -1;
    }
    res = pubnub_await(pbp);
    clk = clock() - clk;
    printf("Publish took %d clicks (%f seconds).\n", (int)clk, ((float)clk)/CLOCKS_PER_SEC);
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
    clk = clock();
    res = pubnub_subscribe(pbp, chan, NULL);
    if (res != PNR_STARTED) {
        printf("pubnub_subscribe() returned unexpected: %d\n", res);
        pubnub_free(pbp);
        return -1;
    }
    res = pubnub_await(pbp);
    clk = clock() - clk;
    printf("Subscribe-connect took %d clicks (%f seconds).\n", (int)clk, ((float)clk)/CLOCKS_PER_SEC);
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
        printf("Subscribing failed with code %d: %s\n", res, pubnub_res_2_string(res));
    }

    res = pubnub_heartbeat(pbp, chan, NULL);
    if (res != PNR_STARTED) {
        printf("pubnub_heartbeat() returned unexpected: %d\n", res);
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
        puts("Heartbeated! Got messages:");
        for (;;) {
            msg = pubnub_get(pbp);
            if (NULL == msg) {
                break;
            }
            puts(msg);
        }
    }
    else {
        printf("Heartbeating failed with code %d: %s\n", res, pubnub_res_2_string(res));
    }

    do_time(pbp);

    puts("Getting history with include_token...");
    res = pubnub_history(pbp, chan, 10, true);
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
        puts("Got history! Elements:");
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
        long, but we have it here to show that we can handle very long
        response if the PUBNUB_DYNAMIC_REPLY_BUFFER is "on".
    */
    if (PUBNUB_DYNAMIC_REPLY_BUFFER) {
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
    }

    puts("Getting where_now presence...");
    res = pubnub_where_now(pbp, pubnub_uuid_get(pbp));
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
    res = pubnub_set_state(pbp, chan, NULL, pubnub_uuid_get(pbp), "{\"x\":5}");
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
    res = pubnub_state_get(pbp, chan, NULL, pubnub_uuid_get(pbp));
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

    puts("List channel group...");
    res = pubnub_list_channel_group(pbp, "channel-group");
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
        puts("Got channel group list!");
        for (;;) {
            msg = pubnub_get_channel(pbp);
            if (NULL == msg) {
                break;
            }
            puts(msg);
        }
    }
    else {
        printf("Getting channel group list failed with code: %d ('%s')\n", res, pubnub_res_2_string(res));
    }
	
    /* We're done */
    if (pubnub_free(pbp) != 0) {
        printf("Failed to free the Pubnub context\n");
    }

    puts("Pubnub sync demo over.");

    return 0;
}
