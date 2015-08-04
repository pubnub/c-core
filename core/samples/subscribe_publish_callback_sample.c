/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_callback.h"

#if defined _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#include <stdio.h>


#if defined _WIN32
static CRITICAL_SECTION mutw;
static HANDLE condw;
static CRITICAL_SECTION mutw_2;
static HANDLE condw_2;
#else
static pthread_mutex_t mutw;
static bool triggered;
static pthread_cond_t condw;
static pthread_mutex_t mutw_2;
static bool triggered_2;
static pthread_cond_t condw_2;
#endif


void sample_callback(pubnub_t *pb, enum pubnub_trans trans, enum pubnub_res result)
{
    switch (trans) {
    case PBTT_SUBSCRIBE:
        /* One could do all handling here, and not signal the `condw`, or use
           some other means to inform others about this event (by, say,
           queueing into some message queue).
        */
        printf("Subscribed, result: %d\n", result);
        break;
    default:
        printf("Some other transaction: result: %d\n", result);
        break;
    }
#if defined _WIN32
    SetEvent(condw);
#else
    pthread_mutex_lock(&mutw);
    triggered = true;
    pthread_cond_signal(&condw);
    pthread_mutex_unlock(&mutw);
#endif
}


void sample_callback_2(pubnub_t *pb, enum pubnub_trans trans, enum pubnub_res result)
{
    switch (trans) {
    case PBTT_PUBLISH:
        printf("Published, result: %d\n", result);
        break;
    default:
        printf("Some other transaction: result: %d\n", result);
        break;
    }
#if defined _WIN32
    SetEvent(condw_2);
#else
    pthread_mutex_lock(&mutw_2);
    triggered_2 = true;
    pthread_cond_signal(&condw_2);
    pthread_mutex_unlock(&mutw_2);
#endif
}

static void start_await(void)
{
#if defined _WIN32	
    ResetEvent(condw);
#else
    pthread_mutex_lock(&mutw);
    triggered = false;
    pthread_mutex_unlock(&mutw);
#endif
}


static void end_await(void)
{
#if defined _WIN32	
    WaitForSingleObject(condw, INFINITE);
#else
    pthread_mutex_lock(&mutw);
    while (!triggered) {
        pthread_cond_wait(&condw, &mutw);
    }
    pthread_mutex_unlock(&mutw);
#endif
}


static void await(void)
{
    start_await();
    end_await();
}



static void await_2(void)
{
#if defined _WIN32	
    ResetEvent(condw_2);
    WaitForSingleObject(condw_2, INFINITE);
#else
    pthread_mutex_lock(&mutw_2);
    triggered_2 = false;
    while (!triggered_2) {
        pthread_cond_wait(&condw_2, &mutw_2);
    }
    pthread_mutex_unlock(&mutw_2);
#endif
}


int main()
{
    char const *msg;
    enum pubnub_res res;
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

#if defined _WIN32
    InitializeCriticalSection(&mutw);
    condw = CreateEvent(NULL, TRUE, FALSE, NULL);
    InitializeCriticalSection(&mutw_2);
    condw_2 = CreateEvent(NULL, TRUE, FALSE, NULL);
#else
    pthread_mutex_init(&mutw, NULL);
    triggered = false;
    pthread_cond_init(&condw, NULL);
    pthread_mutex_init(&mutw_2, NULL);
    triggered_2 = false;
    pthread_cond_init(&condw_2, NULL);
#endif

    pubnub_init(pbp, "demo", "demo");
    pubnub_register_callback(pbp, sample_callback);
    pubnub_init(pbp_2, "demo", "demo");
    pubnub_register_callback(pbp_2, sample_callback_2);

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
    await();
    res = pubnub_last_result(pbp);
    if (res == PNR_STARTED) {
        printf("pubnub_await() returned unexpected: PNR_STARTED(%d)\n", res);
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
    start_await();
    
    puts("-----------------------");
    puts("Publishing...");
    puts("-----------------------");
    /* Since the subscribe is ongoing in the `pbp` context, we can't
       publish on it, so we use a different context to publish
    */
    res = pubnub_publish(pbp_2, chan, "\"Hello world from callback!\"");
    if (res != PNR_STARTED) {
        printf("pubnub_publish() returned unexpected: %d\n", res);
        pubnub_free(pbp);
        pubnub_free(pbp_2);
        return -1;
    }

    puts("Await publish");
    await_2();
    res = pubnub_last_result(pbp_2);
    if (res == PNR_STARTED) {
        printf("pubnub_await() returned unexpected: PNR_STARTED(%d)\n", res);
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
	
	/* Don't need `pbp_2` no more */
    if (pubnub_free(pbp_2) != 0) {
        printf("Failed to free the Pubnub context `pbp_2`\n");
    }
	
	/* Now we await the subscribe on `pbp` */
	puts("Await subscribe");
	end_await();
    res = pubnub_last_result(pbp);
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

	
    /* We're done */
    if (pubnub_free(pbp) != 0) {
        printf("Failed to free the Pubnub context `pbp`\n");
    }

    puts("Pubnub callback demo over.");

    return 0;
}
