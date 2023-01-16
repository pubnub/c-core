/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "core/pubnub_coreapi_ex.h"
#include "pubnub_sync.h"

#include "core/pubnub_helper.h"
#include "core/pubnub_timers.h"

#include <stdio.h>
#include <time.h>

 
static void generate_user_id(pubnub_t* pbp)  
{
    char const*                      user_id_default = "zeka-peka-iz-jendeka";
    struct Pubnub_UUID               uuid;
    static struct Pubnub_UUID_String str_uuid; 
  
    if (0 != pubnub_generate_uuid_v4_random(&uuid)) {
        pubnub_set_user_id(pbp, user_id_default);
    }
    else {
        str_uuid = pubnub_uuid_to_string(&uuid);
        pubnub_set_user_id(pbp, str_uuid.uuid);
        printf("Generated UUID: %s\n", str_uuid.uuid);
    }
}


static void sync_sample_free(pubnub_t* p)
{
    if (PN_CANCEL_STARTED == pubnub_cancel(p)) {
        enum pubnub_res pnru = pubnub_await(p);
        if (pnru != PNR_OK) {
            printf("Awaiting cancel failed: %d('%s')\n",
                   pnru,
                   pubnub_res_2_string(pnru));
        }
    }
    if (pubnub_free(p) != 0) {
        printf("Failed to free the Pubnub context\n");
    }
}


static int do_time(pubnub_t* pbp)
{
    enum pubnub_res res;

    puts("-----------------------");
    puts("Getting time...");
    puts("-----------------------");
    res = pubnub_time(pbp);
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
    }
    if (PNR_OK == res) {
        printf("Gotten time: %s; last time token=%s\n",
               pubnub_get(pbp),
               pubnub_last_time_token(pbp));
    }
    else {
        printf("Getting time failed with code: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
    }

    return 0;
}


int main()
{
    time_t          t0;
    char const*     msg;
    enum pubnub_res res;
    char const*     chan = "hello_world";
    pubnub_t*       pbp  = pubnub_alloc();

    if (NULL == pbp) {
        printf("Failed to allocate Pubnub context!\n");
        return -1;
    }
    pubnub_init(pbp, "demo", "demo");

    pubnub_set_transaction_timeout(pbp, PUBNUB_DEFAULT_SUBSCRIBE_TIMEOUT);

    /* Leave this commented out to use the default - which is
       blocking I/O on most platforms. Uncomment to use non-
       blocking I/O.
    */
    pubnub_set_non_blocking_io(pbp);

    generate_user_id(pbp);

    pubnub_set_auth(pbp, "danaske");

    puts("Publishing...");
    time(&t0);
    res = pubnub_publish(pbp, chan, "\"Hello world from sync!\"");
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
    }
    printf("Publish lasted %lf seconds.\n", difftime(time(NULL), t0));
    if (PNR_OK == res) {
        printf("Published! Response from Pubnub: %s\n",
               pubnub_last_publish_result(pbp));
    }
    else if (PNR_PUBLISH_FAILED == res) {
        printf("Published failed on Pubnub, description: %s\n",
               pubnub_last_publish_result(pbp));
    }
    else {
        printf("Publishing failed with code: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
    }

    puts("Subscribing...");
    time(&t0);
    res = pubnub_subscribe(pbp, chan, NULL);
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
    }
    printf("Subscribe/connect lasted %lf seconds.\n", difftime(time(NULL), t0));
    if (PNR_OK == res) {
        puts("Subscribed!");
    }
    else {
        printf("Subscribing failed with code: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
    }

    time(&t0);
    res = pubnub_subscribe(pbp, chan, NULL);
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
    }
    printf("Subscribe lasted %lf seconds.\n", difftime(time(NULL), t0));
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
        printf("Subscribing failed with code: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
    }

    res = pubnub_heartbeat(pbp, chan, NULL);
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
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
        printf("Heartbeating failed with code: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
    }

    if (do_time(pbp) == -1) {
        return -1;
    }

    puts("Getting history with include_token...");
    res = pubnub_history(pbp, chan, 10, true);
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
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
        printf("Getting history v2 with include_token failed with code: "
               "%d('%s')\n",
               res,
               pubnub_res_2_string(res));
    }

    puts("Getting here_now presence...");
    res = pubnub_here_now(pbp, chan, NULL);
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
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
        printf("Getting here-now presence failed with code: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
    }

    /** Global here_now presence for "demo" subscribe key is _very_
        long, but we have it here to show that we can handle very long
        response if the PUBNUB_DYNAMIC_REPLY_BUFFER is "on".
    */
    if (PUBNUB_DYNAMIC_REPLY_BUFFER) {
        puts("Getting global here_now presence...");
        res = pubnub_global_here_now(pbp);
        if (PNR_STARTED == res) {
            res = pubnub_await(pbp);
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
            printf(
                "Getting global here-now presence failed with code: %d('%s')\n",
                res,
                pubnub_res_2_string(res));
        }
    }

    puts("Getting where_now presence...");
    res = pubnub_where_now(pbp, pubnub_user_id_get(pbp));
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
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
        printf("Getting where-now presence failed with code: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
    }

    puts("Setting state...");
    res = pubnub_set_state(pbp, chan, NULL, pubnub_user_id_get(pbp), "{\"x\":5}");
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
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
        printf("Setting state failed with code: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
    }

    puts("Setting state with options...");
    struct pubnub_set_state_options options = pubnub_set_state_defopts();
    options.heartbeat = true;

    res = pubnub_set_state_ex(pbp, chan, "{\"x\":5}", options);
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
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
        printf("Setting state failed with code: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
    }
    
    puts("Getting state...");
    res = pubnub_state_get(pbp, chan, NULL, pubnub_user_id_get(pbp));
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
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
        printf("Getting state failed with code: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
    }

    puts("List channel group...");
    res = pubnub_list_channel_group(pbp, "channel-group");
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
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
        printf("Getting channel group list failed with code: %d ('%s')\n",
               res,
               pubnub_res_2_string(res));
    }

    puts("Add channel to group");
    res = pubnub_add_channel_to_group(pbp, chan, "channel-group");
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
    }
    if (PNR_OK == res) {
        puts("Got add channel to group response!");
        for (;;) {
            msg = pubnub_get_channel(pbp);
            if (NULL == msg) {
                break;
            }
            puts(msg);
        }
    }
    else {
        printf("Adding channel to group failed with code: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
    }

    puts("Remove channel from group");
    res = pubnub_remove_channel_from_group(pbp, chan, "channel-group");
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
    }
    if (PNR_OK == res) {
        puts("Got remove channel from group response!");
        for (;;) {
            msg = pubnub_get_channel(pbp);
            if (NULL == msg) {
                break;
            }
            puts(msg);
        }
    }
    else {
        printf("Removing channel from group failed with code: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
    }

    puts("Remove channel group");
    res = pubnub_remove_channel_group(pbp, "channel-group");
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
    }
    if (PNR_OK == res) {
        puts("Got remove channel group response!");
        for (;;) {
            msg = pubnub_get_channel(pbp);
            if (NULL == msg) {
                break;
            }
            puts(msg);
        }
    }
    else {
        printf("Removing channel group failed with code: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
    }

    /* We're done, but, keep-alive might be on, so we need to cancel,
     * then free */
    sync_sample_free(pbp);

    puts("Pubnub sync demo over.");

    return 0;
}
