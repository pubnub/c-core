/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_callback.h"

#include "core/pubnub_helper.h"
#include "core/pubnub_timers.h"
#include "core/pubnub_generate_uuid.h"
#include "core/pubnub_free_with_timeout.h"

#if defined _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#include <stdio.h>
#include <time.h>


/** Data that we pass to the Pubnub context and will get back via
    callback. To signal reception of response from Pubnub, that we get
    via callback, we use a condition variable w/pthreads and an Event
    on Windows.
*/
struct UserData {
#if defined _WIN32
    CRITICAL_SECTION mutw;
    HANDLE           condw;
#else
    pthread_mutex_t mutw;
    bool            triggered;
    pthread_cond_t  condw;
#endif
    pubnub_t* pb;
};


void sample_callback(pubnub_t*         pb,
                     enum pubnub_trans trans,
                     enum pubnub_res   result,
                     void*             user_data)
{
    struct UserData* pUserData = (struct UserData*)user_data;

    switch (trans) {
    case PBTT_PUBLISH:
        printf("Published, result: %d('%s')\n", result, pubnub_res_2_string(result));
        break;
    case PBTT_SUBSCRIBE:
        printf("Subscribed, result: %d('%s')\n", result, pubnub_res_2_string(result));
        break;
    case PBTT_LEAVE:
        printf("Left, result: %d('%s')\n", result, pubnub_res_2_string(result));
        break;
    case PBTT_TIME:
        printf("Timed, result: %d('%s')\n", result, pubnub_res_2_string(result));
        break;
    case PBTT_HISTORY:
        printf("Historied, result: %d('%s')\n", result, pubnub_res_2_string(result));
        break;
    case PBTT_HERENOW:
        printf("Here now'ed, result: %d('%s')\n", result, pubnub_res_2_string(result));
        break;
    case PBTT_GLOBAL_HERENOW:
        printf("Global Here now'ed, result: %d('%s')\n",
               result,
               pubnub_res_2_string(result));
        break;
    case PBTT_WHERENOW:
        printf("Where now'ed, result: %d('%s')\n",
               result,
               pubnub_res_2_string(result));
        break;
    case PBTT_SET_STATE:
        printf("State set, result: %d('%s')\n", result, pubnub_res_2_string(result));
        break;
    case PBTT_STATE_GET:
        printf("State gotten, result: %d('%s')\n",
               result,
               pubnub_res_2_string(result));
        break;
    case PBTT_REMOVE_CHANNEL_GROUP:
        printf("Remove channel group, result: %d('%s')\n",
               result,
               pubnub_res_2_string(result));
        break;
    case PBTT_REMOVE_CHANNEL_FROM_GROUP:
        printf("Remove channel from group, result: %d('%s')\n",
               result,
               pubnub_res_2_string(result));
        break;
    case PBTT_ADD_CHANNEL_TO_GROUP:
        printf("Add channel to group, result: %d('%s')\n",
               result,
               pubnub_res_2_string(result));
        break;
    case PBTT_LIST_CHANNEL_GROUP:
        printf("List (members of) channel group, result: %d('%s')\n",
               result,
               pubnub_res_2_string(result));
        break;
    default:
        printf("None?! result: %d('%s')\n", result, pubnub_res_2_string(result));
        break;
    }

#if defined _WIN32
    SetEvent(pUserData->condw);
#else
    pthread_mutex_lock(&pUserData->mutw);
    pUserData->triggered = true;
    pthread_cond_signal(&pUserData->condw);
    pthread_mutex_unlock(&pUserData->mutw);
#endif
}


static enum pubnub_res await(struct UserData* pUserData)
{
#if defined _WIN32
    ResetEvent(pUserData->condw);
    WaitForSingleObject(pUserData->condw, INFINITE);
#else
    pthread_mutex_lock(&pUserData->mutw);
    while (!pUserData->triggered) {
        pthread_cond_wait(&pUserData->condw, &pUserData->mutw);
    }
    pUserData->triggered = false;
    pthread_mutex_unlock(&pUserData->mutw);
#endif
    return pubnub_last_result(pUserData->pb);
}


static void InitUserData(struct UserData* pUserData, pubnub_t* pb)
{
#if defined _WIN32
    InitializeCriticalSection(&pUserData->mutw);
    pUserData->condw = CreateEvent(NULL, TRUE, FALSE, NULL);
#else
    pthread_mutex_init(&pUserData->mutw, NULL);
    pthread_cond_init(&pUserData->condw, NULL);
    pUserData->triggered = false;
#endif
    pUserData->pb = pb;
}


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


static int do_time(pubnub_t* pbp, struct UserData* pUserData)
{
    enum pubnub_res res;

    puts("-----------------------");
    puts("Getting time...");
    puts("-----------------------");
    res = pubnub_time(pbp);
    if (res != PNR_STARTED) {
        printf("pubnub_time() returned unexpected %d('%s')\n",
               res,
               pubnub_res_2_string(res));
        return -1;
    }
    res = await(pUserData);
    if (res == PNR_STARTED) {
        printf("await() returned unexpected: PNR_STARTED(%d)\n", res);
        return -1;
    }

    if (PNR_OK == res) {
        printf("Gotten time: %s; last time token=%s\n",
               pubnub_get(pbp),
               pubnub_last_time_token(pbp));
        printf("Gotten time: %s; last time token=%s\n",
               pubnub_get(pbp),
               pubnub_last_time_token(pbp));
    }
    else {
        printf("Getting time failed with code %d('%s')\n",
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
    struct UserData user_data;
    char const*     chan = "hello_world";
    pubnub_t*       pbp  = pubnub_alloc();
    if (NULL == pbp) {
        printf("Failed to allocate Pubnub context!\n");
        return -1;
    }

    InitUserData(&user_data, pbp);

    pubnub_init(pbp, "demo", "demo");
    pubnub_register_callback(pbp, sample_callback, &user_data);

    pubnub_set_transaction_timeout(pbp, PUBNUB_DEFAULT_NON_SUBSCRIBE_TIMEOUT);

    generate_user_id(pbp);

    puts("-----------------------");
    puts("Publishing...");
    puts("-----------------------");
    time(&t0);
    res = pubnub_publish(pbp, chan, "\"Hello world from callback!\"");
    if (res != PNR_STARTED) {
        printf("pubnub_publish() returned unexpected: %d('%s')\n", res, pubnub_res_2_string(res));
        callback_sample_free(pbp);
        return -1;
    }
    res = await(&user_data);
    printf("Publish lasted %lf seconds.\n", difftime(time(NULL), t0));

    if (res == PNR_STARTED) {
        printf("await() returned unexpected: PNR_STARTED(%d)\n", res);
        callback_sample_free(pbp);
        return -1;
    }
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

    puts("-----------------------");
    puts("Subscribing...");
    puts("-----------------------");
    time(&t0);
    res = pubnub_subscribe(pbp, chan, NULL);
    if (res != PNR_STARTED) {
        printf("pubnub_subscribe() returned unexpected: %d('%s')\n", res, pubnub_res_2_string(res));
        callback_sample_free(pbp);
        return -1;
    }
    res = await(&user_data);
    printf("Subscribe/connect lasted %lf seconds.\n", difftime(time(NULL), t0));
    if (res == PNR_STARTED) {
        printf("await() returned unexpected: PNR_STARTED(%d)\n", res);
        callback_sample_free(pbp);
        return -1;
    }

    if (PNR_OK == res) {
        puts("Subscribed!");
    }
    else {
        printf("Subscribing failed with code: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
    }

    res = pubnub_subscribe(pbp, chan, NULL);
    if (res != PNR_STARTED) {
        printf("pubnub_subscribe() returned unexpected: %d('%s')\n", res, pubnub_res_2_string(res));
        callback_sample_free(pbp);
        return -1;
    }
    res = await(&user_data);
    if (res == PNR_STARTED) {
        printf("await() returned unexpected: PNR_STARTED(%d)\n", res);
        callback_sample_free(pbp);
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
        printf("Subscribing failed with code: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
    }

    /* Getting time */
    if (do_time(pbp, &user_data) == -1) {
        callback_sample_free(pbp);
        return -1;
    }

    puts("-----------------------");
    puts("Getting history v2 with include_token...");
    puts("-----------------------");
    res = pubnub_history(pbp, chan, 10, true);
    if (res != PNR_STARTED) {
        printf("pubnub_history() returned unexpected: %d('%s')\n", res, pubnub_res_2_string(res));
        callback_sample_free(pbp);
        return -1;
    }
    res = await(&user_data);
    if (res == PNR_STARTED) {
        printf("await() returned unexpected: PNR_STARTED(%d)\n", res);
        callback_sample_free(pbp);
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
        printf("Getting historyv2 failed with code: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
    }


    /** Global here_now presence for "demo" subscribe key is _very_
        long, but we have it here to show that we can handle very long
        response if the PUBNUB_DYNAMIC_REPLY_BUFFER is "on".
    */
    if (PUBNUB_DYNAMIC_REPLY_BUFFER) {
        puts("-----------------------");
        puts("Getting global here_now presence...");
        puts("-----------------------");
        res = pubnub_global_here_now(pbp);
        if (res != PNR_STARTED) {
            printf("pubnub_global_here_now() returned unexpected: %d('%s')\n",
                   res,
                   pubnub_res_2_string(res));
            callback_sample_free(pbp);
            return -1;
        }
        res = await(&user_data);
        if (res == PNR_STARTED) {
            printf("await() returned unexpected: PNR_STARTED(%d)\n", res);
            callback_sample_free(pbp);
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
            printf(
                "Getting global here-now presence failed with code: %d('%s')\n",
                res,
                pubnub_res_2_string(res));
        }
    }

    puts("-----------------------");
    puts("Getting where_now presence...");
    puts("-----------------------");
    res = pubnub_where_now(pbp, pubnub_user_id_get(pbp));
    if (res != PNR_STARTED) {
        printf("pubnub_where_now() returned unexpected: %d('%s')\n", res, pubnub_res_2_string(res));
        callback_sample_free(pbp);
        return -1;
    }
    res = await(&user_data);
    if (res == PNR_STARTED) {
        printf("await() returned unexpected: PNR_STARTED(%d)\n", res);
        callback_sample_free(pbp);
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
        printf("Getting where-now presence failed with code: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
    }


    puts("-----------------------");
    puts("Setting state...");
    puts("-----------------------");
    res = pubnub_set_state(pbp, chan, NULL, pubnub_user_id_get(pbp), "{\"x\":5}");
    if (res != PNR_STARTED) {
        printf("pubnub_set_state() returned unexpected: %d('%s')\n", res, pubnub_res_2_string(res));
        callback_sample_free(pbp);
        return -1;
    }
    res = await(&user_data);
    if (res == PNR_STARTED) {
        printf("await() returned unexpected: PNR_STARTED(%d)\n", res);
        callback_sample_free(pbp);
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
        printf("Setting state failed with code: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
    }


    puts("-----------------------");
    puts("Getting state...");
    puts("-----------------------");
    res = pubnub_state_get(pbp, chan, NULL, pubnub_user_id_get(pbp));
    if (res != PNR_STARTED) {
        printf("pubnub_state_get() returned unexpected: %d('%s')\n", res, pubnub_res_2_string(res));
        callback_sample_free(pbp);
        return -1;
    }
    res = await(&user_data);
    if (res == PNR_STARTED) {
        printf("await() returned unexpected: PNR_STARTED(%d)\n", res);
        callback_sample_free(pbp);
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
        printf("Getting state failed with code: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
    }

    puts("-----------------------");
    puts("List channel group...");
    puts("-----------------------");
    res = pubnub_list_channel_group(pbp, "channel-group");
    if (res != PNR_STARTED) {
        printf("pubnub_state_get() returned unexpected: %d('%s')\n", res, pubnub_res_2_string(res));
        callback_sample_free(pbp);
        return -1;
    }
    res = await(&user_data);
    if (res == PNR_STARTED) {
        printf("await() returned unexpected: PNR_STARTED(%d)\n", res);
        callback_sample_free(pbp);
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
        printf("Getting channel group list failed with code: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
    }

    puts("-----------------------");
    puts("Add channel to group");
    puts("-----------------------");
    res = pubnub_add_channel_to_group(pbp, chan, "channel-group");
    if (res != PNR_STARTED) {
        printf("pubnub_add_channel_to_group() returned unexpected: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
        callback_sample_free(pbp);
        return -1;
    }
    res = await(&user_data);
    if (res == PNR_STARTED) {
        printf("await() returned unexpected: PNR_STARTED(%d)\n", res);
        callback_sample_free(pbp);
        return -1;
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
        printf("Adding channel to group failed with code: %d ('%s')\n",
               res,
               pubnub_res_2_string(res));
    }

    puts("-----------------------");
    puts("Remove channel from group");
    puts("-----------------------");
    res = pubnub_remove_channel_from_group(pbp, chan, "channel-group");
    if (res != PNR_STARTED) {
        printf("pubnub_remove_channel_from_group() returned unexpected: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
        callback_sample_free(pbp);
        return -1;
    }
    res = await(&user_data);
    if (res == PNR_STARTED) {
        printf("await() returned unexpected: PNR_STARTED(%d)\n", res);
        callback_sample_free(pbp);
        return -1;
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
        printf("Removing channel from group failed with code: %d ('%s')\n",
               res,
               pubnub_res_2_string(res));
    }

    puts("-----------------------");
    puts("Remove channel group");
    puts("-----------------------");
    res = pubnub_remove_channel_group(pbp, "channel-group");
    if (res != PNR_STARTED) {
        printf("pubnub_remove_channel_group() returned unexpected: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
        callback_sample_free(pbp);
        return -1;
    }
    res = await(&user_data);
    if (res == PNR_STARTED) {
        printf("await() returned unexpected: PNR_STARTED(%d)\n", res);
        callback_sample_free(pbp);
        return -1;
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
        printf("Removing channel group failed with code: %d ('%s')\n",
               res,
               pubnub_res_2_string(res));
    }

    callback_sample_free(pbp);

    puts("Pubnub callback demo over.");

    return 0;
}
