/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_callback.h"

#include "core/pubnub_helper.h"
#include "core/pubnub_timers.h"
#include "core/pubnub_free_with_timeout.h"
#include "core/pubnub_dns_servers.h"

#if defined _WIN32
#include <windows.h>
#include "windows/console_subscribe_paint.h"
#else
#include <pthread.h>
#include "posix/console_subscribe_paint.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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

static short volatile first_subscribe_done;
static short volatile stop;
static char const* m_chan = "hello_world";

static void wait_seconds(unsigned time_in_seconds)
{
    clock_t  start = clock();
    unsigned time_passed_in_seconds;
    do {
        time_passed_in_seconds = (clock() - start) / CLOCKS_PER_SEC;
    } while (time_passed_in_seconds < time_in_seconds);
}

static void sample_free(pubnub_t* p)
{
    pubnub_cancel(p);
    if (pubnub_free_with_timeout(p, 1500) != 0) {
        printf("Failed to free the Pubnub context\n");
    }
    else {
        /* Waits for the context to be released from the processing queue */
        wait_seconds(1);
    }
}
void subscribe_callback(pubnub_t*         pb,
                        enum pubnub_trans trans,
                        enum pubnub_res   result,
                        void*             user_data)
{
    char const* msg;

    switch (trans) {
    case PBTT_SUBSCRIBE:
        /* One could do all handling here, and not signal the `condw`, or use
           some other means to inform others about this event (by, say,
           queueing into some message queue).
        */
        first_subscribe_done = 1;
        paint_text_yellow();
        printf("%sSubscribe callback, result: %d(%s)\n",
               (char*)user_data,
               result,
               pubnub_res_2_string(result));
        if (result == PNR_STARTED) {
            paint_text_red();
            printf("%sSubscribe callback, unexpected: PNR_STARTED(%d)\n",
                   (char*)user_data,
                   result);
            stop = 1;
            return;
        }
        if (PNR_OK == result) {
            paint_text_blue();
            printf("%sSubscribed! Got messages:\n", (char*)user_data);
            paint_text_blue();
            for (;;) {
                msg = pubnub_get(pb);
                if (NULL == msg) {
                    break;
                }
                puts(msg);
            }
        }
        else {
            paint_text_red();
            printf("%sSubscribing failed with code: %d\n", (char*)user_data, result);
        }
        if (!stop && (result != PNR_CANCELLED)) {
            char chan[30];
            snprintf(chan, sizeof chan, "%s%s", (char*)user_data, m_chan);
            /* The "real" subscribe, with the just acquired time token */
            result = pubnub_subscribe(pb, chan, NULL);
            if (result != PNR_STARTED) {
                paint_text_red();
                printf("%spubnub_subscribe() returned unexpected: %d\n",
                       (char*)user_data,
                       result);
                stop = 1;
                return;
            }
        }
        break;
    default:
        paint_text_red();
        stop = 1;
        printf("%sTransaction %d callback(subscribe) result: %d\n",
               (char*)user_data,
               trans,
               result);
        break;
    }
    reset_text_paint();
}


void publish_callback(pubnub_t*         pb,
                      enum pubnub_trans trans,
                      enum pubnub_res   result,
                      void*             user_data)
{
    time_t t;
    /* Intializes random number generator */
    srand((unsigned)time(&t));

    switch (trans) {
    case PBTT_PUBLISH:
        paint_text_green();
        printf("%sPublish callback, result: %d\n", (char*)user_data, result);
        if (result == PNR_STARTED) {
            paint_text_red();
            printf("%sawait() returned unexpected: PNR_STARTED(%d)\n",
                   (char*)user_data,
                   result);
            stop = 1;
            return;
        }
        if (PNR_OK == result) {
            paint_text_green();
            printf("%sPublished! Response from Pubnub: %s\n",
                   (char*)user_data,
                   pubnub_last_publish_result(pb));
        }
        else if (PNR_PUBLISH_FAILED == result) {
            paint_text_red();
            printf("%sPublished failed on Pubnub, description: %s\n",
                   (char*)user_data,
                   pubnub_last_publish_result(pb));
        }
        else {
            paint_text_red();
            printf("%sPublishing failed with code:%d(%s)\n",
                   (char*)user_data,
                   result,
                   pubnub_res_2_string(result));
        }
        if (result != PNR_CANCELLED) {
            /* Seed string */
            char s[100] = "\"pub";
            char chan[30];
            snprintf(chan, sizeof chan, "%s%s", (char*)user_data, m_chan);
            if (!stop && (rand() % 10 > 5)) {
                paint_text_green();
                puts("-----------------------");
                puts("Publishing...");
                puts("-----------------------");

                snprintf(
                    s,
                    sizeof s,
                    "\"%sHello world from subscribe-publish callback sample!\"",
                    (char*)user_data);
                result = pubnub_publish(pb, chan, s);
            }
            else {
                snprintf(s, sizeof s, "\"%s\"", (char*)user_data);
                result = pubnub_publish(pb, chan, s);
            }
            if (result != PNR_STARTED) {
                paint_text_red();
                printf("%spubnub_publish() returned unexpected: %d\n",
                       (char*)user_data,
                       result);
                stop = 1;
            }
        }
        break;
    default:
        paint_text_red();
        stop = 1;
        printf("%sTransaction %d callback(publish): result: %d\n",
               (char*)user_data,
               trans,
               result);
        break;
    }
    reset_text_paint();
}


int main()
{
    enum pubnub_res            res;
    char                       chan1[30];
    struct pubnub_ipv4_address o_ipv4[3];
    pubnub_t*                  pbp   = pubnub_alloc();
    pubnub_t*                  pbp_2 = pubnub_alloc();

    if (NULL == pbp) {
        paint_text_red();
        printf("Failed to allocate Pubnub context1!\n");
        return -1;
    }
    if (NULL == pbp_2) {
        paint_text_red();
        printf("Failed to allocate Pubnub context2!\n");
        return -1;
    }

    pubnub_init(pbp, "demo", "demo");
    pubnub_register_callback(pbp, subscribe_callback, "CH1");
    pubnub_init(pbp_2, "demo", "demo");
    pubnub_register_callback(pbp_2, publish_callback, "CH1");

    if (pubnub_dns_read_system_servers_ipv4(o_ipv4, 3) > 0) {
        if (pubnub_dns_set_primary_server_ipv4(o_ipv4[0]) != 0) {
            paint_text_red();
            printf("Failed to set DNS server from the sistem register!\n");
            return -1;
        }
    }
    else {
        paint_text_yellow();
        printf("Failed to read system DNS server, will use default %s\n",
               PUBNUB_DEFAULT_DNS_SERVER);
    }

    pubnub_set_transaction_timeout(pbp, 5000);

    paint_text_white();
    puts("-----------------------");
    puts("Subscribing1...");
    puts("-----------------------");

    snprintf(chan1, sizeof chan1, "CH1%s", m_chan);
    /* First subscribe, to get the time token */
    res = pubnub_subscribe(pbp, chan1, NULL);
    if (res != PNR_STARTED) {
        printf("pubnub_subscribe1() returned unexpected: %d\n", res);
        sample_free(pbp);
        sample_free(pbp_2);
        return -1;
    }

    /* FIXME: To be 100% correct, `first_subscribe_done` should be an
       atomic variable, not just volatile...
    */
    do {
    } while (!first_subscribe_done);

    if (!stop) {
        paint_text_white();
        puts("-----------------------");
        puts("Publishing1...");
        puts("-----------------------");
        res = pubnub_publish(pbp_2,
                             chan1,
                             "\"[1]Hello world from 'subscribe-publish from callback' sample!\"");
        if (res != PNR_STARTED) {
            printf("pubnub_publish1() returned unexpected: %d\n", res);
            stop = 1;
        }
    }

    wait_seconds(200);

    /* We're done, but, if keep-alive is on, we can't free,
       we need to cancel first...
     */
    sample_free(pbp_2);
    sample_free(pbp);

    paint_text_white();
    puts("Pubnub subscribe-publish callback demo over.\n");

    return 0;
}
