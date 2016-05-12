/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_callback.h"

#if defined _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#include <stdio.h>


/** Data that we pass to the Pubnub context and will get back via
    callback. To signal reception of response from Pubnub, that we get
    via callback, we use a condition variable w/pthreads and an Event
    on Windows.
*/
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
    case PBTT_SUBSCRIBE:
        /* One could do all handling here, and not signal the `condw`, or use
           some other means to inform others about this event (by, say,
           queueing into some message queue).
        */
        printf("Subscribed, result: %d\n", result);
        break;
    case PBTT_PUBLISH:
        printf("Published, result: %d\n", result);
        break;
    default:
        printf("Some other transaction: result: %d\n", result);
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


static void start_await(struct UserData *pUserData)
{
#if defined _WIN32	
    ResetEvent(pUserData->condw);
#else
    pthread_mutex_lock(&pUserData->mutw);
    pUserData->triggered = false;
    pthread_mutex_unlock(&pUserData->mutw);
#endif
}


static enum pubnub_res end_await(struct UserData *pUserData)
{
#if defined _WIN32	
    WaitForSingleObject(pUserData->condw, INFINITE);
#else
    pthread_mutex_lock(&pUserData->mutw);
    while (!pUserData->triggered) {
        pthread_cond_wait(&pUserData->condw, &pUserData->mutw);
    }
    pthread_mutex_unlock(&pUserData->mutw);
#endif
    return pubnub_last_result(pUserData->pb);
}


static enum pubnub_res await(struct UserData *pUserData)
{
    start_await(pUserData);
    return end_await(pUserData);
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
    struct UserData user_data_2;
    char const *chan = "hello_world";
    pubnub_t *pbp = pubnub_alloc();
    pubnub_t *pbp_2 = pubnub_alloc();
    if (NULL == pbp) {
        printf("Failed to allocate Pubnub context!\n");
        return -1;
    }
    if (NULL == pbp_2) {
        printf("Failed to allocate Pubnub context!\n");
        return -1;
    }

    InitUserData(&user_data, pbp);
    InitUserData(&user_data_2, pbp_2);

    pubnub_init(pbp, "demo", "demo");
    pubnub_register_callback(pbp, sample_callback, &user_data);
    pubnub_init(pbp_2, "demo", "demo");
    pubnub_register_callback(pbp_2, sample_callback, &user_data_2);

    puts("-----------------------");
    puts("Subscribing...");
    puts("-----------------------");
	
    /* First subscribe, to get the time token */
    res = pubnub_subscribe(pbp, chan, NULL);
    if (res != PNR_STARTED) {
        printf("pubnub_subscribe() returned unexpected: %d\n", res);
        pubnub_free(pbp);
        pubnub_free(pbp_2);
        return -1;
    }

    puts("Await subscribe");
    res = await(&user_data);
    if (res == PNR_STARTED) {
        printf("await() returned unexpected: PNR_STARTED(%d)\n", res);
        pubnub_free(pbp);
        pubnub_free(pbp_2);
        return -1;
    }
    if (PNR_OK == res) {
        puts("Subscribed!");
    }
    else {
        printf("Subscribing failed with code: %d\n", res);
    }

    /* The "real" subscribe, with the just acquired time token */
    res = pubnub_subscribe(pbp, chan, NULL);
    if (res != PNR_STARTED) {
        printf("pubnub_subscribe() returned unexpected: %d\n", res);
        pubnub_free(pbp);
        pubnub_free(pbp_2);
        return -1;
    }
	
    /* Don't do "full" await here, because we didn't publish anything yet! */
    start_await(&user_data);
    
    puts("-----------------------");
    puts("Publishing...");
    puts("-----------------------");

    /* Since the subscribe is ongoing in the `pbp` context, we can't
       publish on it, so we use a different context to publish
    */
    res = pubnub_publish(pbp_2, chan, "\"Hello world from subscribe-publish callback sample!\"");
    if (res != PNR_STARTED) {
        printf("pubnub_publish() returned unexpected: %d\n", res);
        pubnub_free(pbp);
        pubnub_free(pbp_2);
        return -1;
    }

    puts("Await publish");
    res = await(&user_data_2);
    if (res == PNR_STARTED) {
        printf("await() returned unexpected: PNR_STARTED(%d)\n", res);
        pubnub_free(pbp);
        pubnub_free(pbp_2);
        return -1;
    }
    if (PNR_OK == res) {
        printf("Published! Response from Pubnub: %s\n", pubnub_last_publish_result(pbp_2));
    }
    else if (PNR_PUBLISH_FAILED == res) {
        printf("Published failed on Pubnub, description: %s\n", pubnub_last_publish_result(pbp_2));
    }
    else {
        printf("Publishing failed with code: %d\n", res);
    }

    /* Now we await the subscribe on `pbp` */
    puts("Await subscribe");
    res = end_await(&user_data);
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

	
    /* We're done */
    if (pubnub_free(pbp_2) != 0) {
        printf("Failed to free the Pubnub context `pbp_2`\n");
    }
    if (pubnub_free(pbp) != 0) {
        printf("Failed to free the Pubnub context `pbp`\n");
    }

    puts("Pubnub subscribe-publish callback demo over.");

    return 0;
}
