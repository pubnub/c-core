/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_callback.h"

#include "core/pubnub_callback_subscribe_loop.h"
#include "core/pubnub_helper.h"
#include "core/pubnub_free_with_timeout.h"
#include "core/pubnub_dns_servers.h"

#if defined _WIN32
#include <windows.h>
#include "windows/console_subscribe_paint.h"
#else
#include "posix/console_subscribe_paint.h"
#endif

#include <stdio.h>
#include <time.h>


static volatile int m_stop;


static void subloop_callback(pubnub_t* pbp, char const* message, enum pubnub_res result)
{
    PUBNUB_UNUSED(pbp);
    if (PNR_OK == result) {
        paint_text_white();
        printf("Received message '%s'\n", message);
    }
    else {
        paint_text_red();
        printf("Subscribe failed with code: %d('%s')\n", result, pubnub_res_2_string(result));
    }
    reset_text_paint();
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
        printf("Publish callback, result: %d('%s')\n", result, pubnub_res_2_string(result));
        if (result == PNR_STARTED) {
            printf("await() returned unexpected: PNR_STARTED(%d)\n", result);
            m_stop = 1;
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
            printf("Publishing failed with code: %d('%s')\n", result, pubnub_res_2_string(result));
        }
        if (result != PNR_CANCELLED) {
            /* Intializes random number generator */
            srand((unsigned)time(&t));
            if (!m_stop && (rand() % 10 > 5)) {
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
                printf("pubnub_publish() returned unexpected: %d('%s')\n",
                       result,
                       pubnub_res_2_string(result));
                m_stop = 1;
            }
        }
        break;
    default:
        paint_text_red();
        m_stop = 1;
        printf("Transaction %d callback: result: %d('%s')\n",
               trans,
               result,
               pubnub_res_2_string(result));
        break;
    }
    reset_text_paint();
    return;
}


static void wait_seconds(double time_in_seconds)
{
    time_t start = time(NULL);
    double time_passed_in_seconds;
    do {
        time_passed_in_seconds = difftime(time(NULL), start);
    } while (time_passed_in_seconds < time_in_seconds);
}


static void callback_sample_free(pubnub_t* pb)
{
    if (0 != pubnub_free_with_timeout(pb, 1000)) {
        puts("Failed to free the context in due time");
    }
    else {
        /* Waits until the context is released from the processing queue */
        wait_seconds(2);
    }
}


int main()
{
    const unsigned    minutes_in_loop = 1;
    char const*       chan            = "hello_world";
    pubnub_t*         pbp             = pubnub_alloc();
    pubnub_t*         pbp_2           = pubnub_alloc();
    enum pubnub_res   result;
    struct pubnub_ipv4_address o_ipv4[3];
    pubnub_subloop_t* pbsld;

    if (NULL == pbp) {
        printf("Failed to allocate Pubnub context!\n");
        return -1;
    }
    pubnub_init(pbp, "demo", "demo");
    pubnub_init(pbp_2, "demo", "demo");

    pubnub_set_user_id(pbp, "demo");
    pubnub_set_user_id(pbp_2, "demo_2");

    pubnub_register_callback(pbp_2, publish_callback, (void*)chan);

	paint_text_white();
    //! [Define subscribe loop]
    pbsld = pubnub_subloop_define(
        pbp, chan, pubnub_subscribe_defopts(), subloop_callback);
    if (NULL == pbsld) {
        printf("Defining a subscribe loop failed\n");
        callback_sample_free(pbp);
        callback_sample_free(pbp_2);
        return -1;
    }

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
        printf("pubnub_publish() returned unexpected: %d('%s')\n",
               result,
               pubnub_res_2_string(result));
        m_stop = 1;
    }

    //    do{
    //    }while(!m_stop);
    wait_seconds(200);
    //    just_wait_minutes(minutes_in_loop);

    //! [Stop a subscribe loop]
    pubnub_subloop_stop(pbsld);
    //! [Start Subscribe loop]

    //! [Release a subscribe loop]
    pubnub_subloop_undef(pbsld);
    //! [Release Subscribe loop]

    callback_sample_free(pbp_2);
    callback_sample_free(pbp);

    paint_text_white();
    puts("Pubnub callback subloop demo over.");

    return 0;
}
