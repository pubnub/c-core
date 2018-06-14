/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_callback.h"

#include "core/pubnub_callback_subscribe_loop.h"
#include "core/pubnub_helper.h"
#include "core/pubnub_free_with_timeout.h"

#if defined _WIN32
#include <windows.h>
#include "windows/console_subscribe_paint.h"
/* FIXME: It's ugly that we have to declare these here, far, far, away
   from the macros in the above header that use them...
 */
HANDLE m_hstdout_;
WORD   m_wOldColorAttrs_;
#else
#include "posix/console_subscribe_paint.h"
#endif

#include <stdio.h>
#include <time.h>


static volatile int stop;


static void subloop_callback(pubnub_t* pbp, char const* message, enum pubnub_res result)
{
    PUBNUB_UNUSED(pbp);
    if (PNR_OK == result) {
        printf("Received message '%s'\n", message);
    }
    else {
        printf("Subscribe failed with code: %d\n", result);
    }
}


void publish_callback(pubnub_t*         pb,
                      enum pubnub_trans trans,
                      enum pubnub_res   result,
                      void*             user_data)
{
    time_t t;

    switch (trans) {
    case PBTT_PUBLISH:
        paint_text_green();
        printf("Publish callback, result: %d\n", result);
        if (result == PNR_STARTED) {
            printf("await() returned unexpected: PNR_STARTED(%d)\n", result);
            stop = 1;
            return;
        }
        if (PNR_OK == result) {
            printf("Published! Response from Pubnub: %s\n",
                   pubnub_last_publish_result(pb));
        }
        else if (PNR_PUBLISH_FAILED == result) {
            paint_text_red();
            printf("Published failed on Pubnub, description: %s\n",
                   pubnub_last_publish_result(pb));
        }
        else {
            paint_text_red();
            printf("Publishing failed with code: %d\n", result);
        }
        if (result != PNR_CANCELLED) {
            /* Intializes random number generator */
            srand((unsigned)time(&t));
            if (!stop && (rand() % 10 > 5)) {
                paint_text_green();
                puts("-----------------------");
                puts("Publishing...");
                puts("-----------------------");

                result = pubnub_publish(
                    pb,
                    (char*)user_data,
                    "\"Hello world from subscribe-publish callback sample!\"");
            }
            else {
                result = pubnub_publish(pb, (char*)user_data, "\"\"");
            }
            if (result != PNR_STARTED) {
                paint_text_red();
                printf("pubnub_publish() returned unexpected: %d\n", result);
                stop = 1;
            }
        }
        break;
    default:
        paint_text_red();
        stop = 1;
        printf("Transaction %d callback: result: %d\n", trans, result);
        break;
    }
    reset_text_paint();
    return;
}


static void wait_seconds(unsigned time_in_seconds)
{
    clock_t  start = clock();
    unsigned time_passed_in_seconds;
    do {
        time_passed_in_seconds = (clock() - start) / CLOCKS_PER_SEC;
    } while (time_passed_in_seconds < time_in_seconds);
}


static void sample_free(pubnub_t* pb)
{
    /* We're done, but, if keep-alive is on, we can't free,
       we need to cancel first...
     */
    pubnub_cancel(pb);
    if (0 != pubnub_free_with_timeout(pb, 1000)) {
        puts("Failed to free the context in due time");
    }
    else {
        /* Waits until the context is released from the processing queue */
        wait_seconds(1);
    }
}


int main()
{
    const unsigned    minutes_in_loop = 1;
    char const*       chan            = "hello_world";
    pubnub_t*         pbp             = pubnub_alloc();
    pubnub_t*         pbp_2           = pubnub_alloc();
    enum pubnub_res   result;
    pubnub_subloop_t* pbsld;

    if (NULL == pbp) {
        printf("Failed to allocate Pubnub context!\n");
        return -1;
    }
    pubnub_init(pbp, "demo", "demo");
    pubnub_init(pbp_2, "demo", "demo");
    pubnub_register_callback(pbp_2, publish_callback, (void*)chan);

    //! [Define subscribe loop]
    pbsld = pubnub_subloop_define(
        pbp, chan, pubnub_subscribe_defopts(), subloop_callback);
    if (NULL == pbsld) {
        printf("Defining a subscribe loop failed\n");
        pubnub_free(pbp);
        pubnub_free(pbp_2);
        /* Waits until the context is released from the processing queue */
        wait_seconds(1);
        return -1;
    }
    //! [Define subscribe loop]

    printf("Entering subscribe loop for channel '%s' for %d minutes...\n",
           chan,
           minutes_in_loop);

    //! [Start a subscribe loop]
    pubnub_subloop_start(pbsld);
    //! [Start Subscribe loop]
    result = pubnub_publish(
        pbp_2, chan, "\"Hello world from subscribe-publish callback sample!\"");
    if (result != PNR_STARTED) {
        paint_text_yellow();
        printf("pubnub_publish() returned unexpected: %d\n", result);
        stop = 1;
    }

    //    do{
    //    }while(!stop);
    wait_seconds(200);
    //    just_wait_minutes(minutes_in_loop);

    //! [Stop a subscribe loop]
    pubnub_subloop_stop(pbsld);
    //! [Start Subscribe loop]

    //! [Release a subscribe loop]
    pubnub_subloop_undef(pbsld);
    //! [Release Subscribe loop]

    sample_free(pbp_2);
    sample_free(pbp);
    puts("Pubnub callback subloop demo over.");

    return 0;
}
