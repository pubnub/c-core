/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_callback.h"

#include "core/pubnub_callback_subscribe_loop.h"
#include "core/pubnub_timers.h"
#include "core/pubnub_helper.h"
#include "core/pubnub_log.h"
#include "core/pubnub_free_with_timeout.h"
#include "core/pubnub_mutex.h"
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
#include <time.h>


static volatile int m_stop;
static char         m_message[50][200] pubnub_guarded_by(m_lock);
static short m_head pubnub_guarded_by(m_lock);
static short m_tail pubnub_guarded_by(m_lock);
pubnub_mutex_static_decl_and_init(m_lock);


static void subloop_callback(pubnub_t* pbp, char const* message, enum pubnub_res result)
{
    PUBNUB_UNUSED(pbp);
    if (PNR_OK == result) {
        paint_text_blue();
        printf("Received message: '%s'\n", message);
    }
    else {
        paint_text_red();
        printf("Subscribe failed with code:%d('%s')\n",
               result,
               pubnub_res_2_string(result));
    }
    reset_text_paint();
}


void publish_callback(pubnub_t*         pb,
                      enum pubnub_trans trans,
                      enum pubnub_res   result,
                      void*             user_data)
{
    switch (trans) {
    case PBTT_PUBLISH:
        paint_text_green();
        printf("Publish callback, result: %d('%s')\n", result, pubnub_res_2_string(result));
        if (result == PNR_STARTED) {
            paint_text_red();
            printf("await() returned unexpected: PNR_STARTED(%d)\n", result);
            m_stop = 1;
            return;
        }
        if (PNR_OK == result) {
            paint_text_green();
            printf("Published! Response from Pubnub: %s\n",
                   pubnub_last_publish_result(pb));
        }
        else if (PNR_PUBLISH_FAILED == result) {
            paint_text_red();
            printf("Publish failed on Pubnub, description: %s\n",
                   pubnub_last_publish_result(pb));
        }
        else {
            paint_text_red();
            printf("Publishing failed with code: %d('%s')\n",
                   result,
                   pubnub_res_2_string(result));
        }
        if (result != PNR_CANCELLED) {
            pubnub_mutex_lock(m_lock);
            if (m_tail != m_head) {
                puts("-----------------------");
                puts("Publishing...");
                puts("-----------------------");
                result = pubnub_publish(pb, (char*)user_data, m_message[m_tail]);
                if (result == PNR_STARTED) {
                    m_tail =
                        (m_tail + 1) % (sizeof m_message / sizeof m_message[0]);
                }
                else {
                    paint_text_red();
                    printf("pubnub_publish() from callback returned "
                           "unexpected:%d('%s')\n",
                           result,
                           pubnub_res_2_string(result));
                    m_stop = 1;
                }
            }
            pubnub_mutex_unlock(m_lock);
        }
        break;
    default:
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


static void wait_miliseconds(unsigned time_in_miliseconds)
{
    clock_t  start = clock();
    unsigned time_passed_in_miliseconds;
    do {
        time_passed_in_miliseconds = (clock() - start) / (CLOCKS_PER_SEC / 1000);
    } while (time_passed_in_miliseconds < time_in_miliseconds);
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
    if (0 != pubnub_free_with_timeout(pb, 1500)) {
        puts("Failed to free the context in due time");
    }
    else {
        /* Waits until the context is released from the processing queue */
        wait_seconds(2);
    }
}


int main()
{
    char const*                chan  = "hello_world";
    pubnub_t*                  pbp   = pubnub_alloc();
    pubnub_t*                  pbp_2 = pubnub_alloc();
    enum pubnub_res            result;
    pubnub_subloop_t*          pbsld;
    struct pubnub_ipv4_address o_ipv4[3];
    time_t                     t;
    time_t                     seconds_in_loop = 200;
    time_t                     start           = time(NULL);
    unsigned                   i               = 0;

    if ((NULL == pbp) || (NULL == pbp_2)) {
        printf("Failed to allocate Pubnub context(1 or 2)!\n");
        return -1;
    }
    pubnub_init(pbp, "demo", "demo");
    pubnub_init(pbp_2, "demo", "demo");

    pubnub_set_user_id(pbp, "demo");
    pubnub_set_user_id(pbp_2, "demo_2");

    pubnub_register_callback(pbp_2, publish_callback, (void*)chan);

    pubnub_mutex_init_static(m_lock);

    paint_text_white();
    //! [Define subscribe loop]
    pbsld = pubnub_subloop_define(
        pbp, chan, pubnub_subscribe_defopts(), subloop_callback);
    if (NULL == pbsld) {
        paint_text_red();
        printf("Defining a subscribe loop failed\n");
        callback_sample_free(pbp);
        callback_sample_free(pbp_2);
        paint_text_white();
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

    pubnub_set_transaction_timeout(pbp, 10000);
    //! [Define subscribe loop]

    printf("Entering subscribe loop for channel '%s'!\n", chan);

    //! [Start a subscribe loop]
    pubnub_subloop_start(pbsld);
    //! [Start Subscribe loop]
    /* Intializes random number generator */
    srand((unsigned)time(&t));

    while (!m_stop) {
        char dest[1000];

        pubnub_mutex_lock(m_lock);
        if (m_tail == m_head) {
            pubnub_mutex_unlock(m_lock);
            snprintf(dest, sizeof dest, "\"%d\"", ++i);
            result = pubnub_publish(pbp_2, chan, dest);
            if (result == PNR_IN_PROGRESS) {
                paint_text_blue();
                printf("for dest_string = '%s' PUBLISH_IN_PROGRESS\n", dest);
            }
            else if (result != PNR_STARTED) {
                paint_text_red();
                printf("pubnub_publish() returned unexpected:%d('%s')\n",
                       result,
                       pubnub_res_2_string(result));
                m_stop = 1;
            }
        }
        else {
            pubnub_mutex_unlock(m_lock);
        }
        if (!m_stop && (rand() % 20 < 7)) {
            ++i;
            pubnub_mutex_lock(m_lock);
            if (((m_head + 1) % (sizeof m_message / sizeof m_message[0])) != m_tail) {
                paint_text_yellow();
                snprintf(
                    m_message[m_head],
                    sizeof m_message[m_head],
                    "\"%d-hello world from subscribe publish callback queue!\"",
                    i);
                printf("Enqueuing a message: %s\n", m_message[m_head]);
                m_head = (m_head + 1) % (sizeof m_message / sizeof m_message[0]);
                pubnub_mutex_unlock(m_lock);
            }
            else {
                pubnub_mutex_unlock(m_lock);
                paint_text_red();
                printf("\n---------------->Queue for 'publish' is "
                       "full!------------------------->\n\n");
            }
        }
        reset_text_paint();

        if (difftime(time(NULL), start) > seconds_in_loop) {
            break;
        }

        wait_miliseconds(300);
    }
    pubnub_mutex_destroy(m_lock);

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
