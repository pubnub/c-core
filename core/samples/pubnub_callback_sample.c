/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_callback.h"

#if defined _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#include <stdio.h>


struct UserData {
#if defined _WIN32
    CRITICAL_SECTION mutw;
    HANDLE condw;
#else
    pthread_mutex_t mutw;
    bool triggered;
    pthread_cond_t condw;
#endif
    pubnub_t *pb;
};

void sample_callback(pubnub_t *pb, enum pubnub_trans trans, enum pubnub_res result, void *user_data)
{
    struct UserData *pUserData = (struct UserData*)user_data;
    
    switch (trans) {
    case PBTT_PUBLISH:
        printf("Published, result: %d\n", result);
        break;
    case PBTT_SUBSCRIBE:
        printf("Subscribed, result: %d\n", result);
        break;
    case PBTT_LEAVE:
        printf("Left, result: %d\n", result);
        break;
    case PBTT_TIME:
        printf("Timed, result: %d\n", result);
        break;
    case PBTT_HISTORY:
        printf("Historied, result: %d\n", result);
        break;
    case PBTT_HISTORYV2:
        printf("Historied v2, result: %d\n", result);
        break;
    case PBTT_HERENOW:
        printf("Here now'ed, result: %d\n", result);
        break;
    case PBTT_GLOBAL_HERENOW:
        printf("Global Here now'ed, result: %d\n", result);
        break;
    case PBTT_WHERENOW:
        printf("Where now'ed, result: %d\n", result);
        break;
    case PBTT_SET_STATE:
        printf("State set, result: %d\n", result);
        break;
    case PBTT_STATE_GET:
        printf("State gotten, result: %d\n", result);
        break;
    default:
        printf("None?! result: %d\n", result);
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


static enum pubnub_res await(struct UserData *pUserData)
{
#if defined _WIN32	
    ResetEvent(pUserData->condw);
    WaitForSingleObject(pUserData->condw, INFINITE);
#else
    pthread_mutex_lock(&pUserData->mutw);
    pUserData->triggered = false;
    while (!pUserData->triggered) {
        pthread_cond_wait(&pUserData->condw, &pUserData->mutw);
    }
    pthread_mutex_unlock(&pUserData->mutw);
#endif
    return pubnub_last_result(pUserData->pb);
}


static void InitUserData(struct UserData *pUserData, pubnub_t *pb)
{
#if defined _WIN32
    InitializeCriticalSection(&pUserData->mutw);
    pUserData->condw = CreateEvent(NULL, TRUE, FALSE, NULL);
#else
    pthread_mutex_init(&pUserData->mutw, NULL);
    pthread_cond_init(&pUserData->condw, NULL);
#endif
    pUserData->pb = pb;
}


int main()
{
    char const *msg;
    enum pubnub_res res;
    struct UserData user_data;
    char const *chan = "hello_world";
    pubnub_t *pbp = pubnub_alloc();
    if (NULL == pbp) {
        printf("Failed to allocate Pubnub context!\n");
        return -1;
    }

    InitUserData(&user_data, pbp);

    pubnub_init(pbp, "demo", "demo");
    pubnub_register_callback(pbp, sample_callback, &user_data);

    puts("-----------------------");
    puts("Publishing...");
    puts("-----------------------");
    res = pubnub_publish(pbp, chan, "\"Hello world from callback!\"");
    if (res != PNR_STARTED) {
        printf("pubnub_publish() returned unexpected: %d\n", res);
        pubnub_free(pbp);
        return -1;
    }

    res = await(&user_data);
    if (res == PNR_STARTED) {
        printf("await() returned unexpected: PNR_STARTED(%d)\n", res);
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

    puts("-----------------------");
    puts("Subscribing...");
    puts("-----------------------");
    res = pubnub_subscribe(pbp, chan, NULL);
    if (res != PNR_STARTED) {
        printf("pubnub_subscribe() returned unexpected: %d\n", res);
        pubnub_free(pbp);
        return -1;
    }

    res = await(&user_data);
    if (res == PNR_STARTED) {
        printf("await() returned unexpected: PNR_STARTED(%d)\n", res);
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

    res = await(&user_data);
    if (res == PNR_STARTED) {
        printf("await() returned unexpected: PNR_STARTED(%d)\n", res);
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
    res = await(&user_data);
    if (res == PNR_STARTED) {
        printf("await() returned unexpected: PNR_STARTED(%d)\n", res);
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
    res = await(&user_data);
    if (res == PNR_STARTED) {
        printf("await() returned unexpected: PNR_STARTED(%d)\n", res);
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

    puts("Getting history v2 with include_token...");
    res = pubnub_historyv2(pbp, chan, NULL, 10, true);
    if (res != PNR_STARTED) {
        printf("pubnub_historyv2() returned unexpected: %d\n", res);
        pubnub_free(pbp);
        return -1;
    }
    res = await(&user_data);
    if (res == PNR_STARTED) {
        printf("await() returned unexpected: PNR_STARTED(%d)\n", res);
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
        printf("Getting historyv2 failed with code: %d\n", res);
    }

    /* We're done */
    if (pubnub_free(pbp) != 0) {
        printf("Failed to free the Pubnub context\n");
    }

    puts("Pubnub callback demo over.");

    return 0;
}
