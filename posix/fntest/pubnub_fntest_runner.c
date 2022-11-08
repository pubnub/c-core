/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "core/fntest/pubnub_fntest.h"

#include "core/fntest/pubnub_fntest_basic.h"
#include "core/fntest/pubnub_fntest_medium.h"

#include "core/pubnub_alloc.h"
#include "core/pubnub_pubsubapi.h"
#include "core/srand_from_pubnub_time.h"
#include "core/pubnub_log.h"

#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


typedef void* (*PF_Test_T)(void* pResult);

struct TestData {
    PF_Test_T           pf;
    char const*         name;
    pthread_t           pth;
    enum PNFNTestResult result;
};


#define LIST_TEST(tstname)                                                     \
    {                                                                          \
        pnfn_test_##tstname, #tstname, (pthread_t)0, trIndeterminate           \
    }


static struct TestData m_aTest[] = {
    LIST_TEST(simple_connect_and_send_over_single_channel),
    LIST_TEST(connect_and_send_over_several_channels_simultaneously),
    LIST_TEST(simple_connect_and_send_over_single_channel_in_group),
    LIST_TEST(connect_and_send_over_several_channels_in_group_simultaneously),
    LIST_TEST(connect_and_send_over_channel_in_group_and_single_channel_simultaneously),
    LIST_TEST(connect_and_send_over_channel_in_group_and_multi_channel_simultaneously),
    LIST_TEST(simple_connect_and_receiver_over_single_channel),
    LIST_TEST(connect_and_receive_over_several_channels_simultaneously),
    LIST_TEST(simple_connect_and_receiver_over_single_channel_in_group),
    LIST_TEST(connect_and_receive_over_several_channels_in_group_simultaneously),
    LIST_TEST(connect_and_receive_over_channel_in_group_and_single_channel_simultaneously),
    LIST_TEST(connect_and_receive_over_channel_in_group_and_multi_channel_simultaneously),
    /*	LIST_TEST(broken_connection_test),
        LIST_TEST(broken_connection_test_multi),
        LIST_TEST(broken_connection_test_group),
        LIST_TEST(broken_connection_test_multi_in_group),
        LIST_TEST(broken_connection_test_group_in_group_out),
        LIST_TEST(broken_connection_test_group_multichannel_out),*/

    LIST_TEST(complex_send_and_receive_over_several_channels_simultaneously),
    LIST_TEST(complex_send_and_receive_over_channel_plus_group_simultaneously),
    LIST_TEST(connect_disconnect_and_connect_again),
    /*	LIST_TEST(connect_disconnect_and_connect_again_group),
        LIST_TEST(connect_disconnect_and_connect_again_combo),
        LIST_TEST(wrong_api_usage),*/
};

#define TEST_COUNT (sizeof m_aTest / sizeof m_aTest[0])


static void srand_from_pubnub(char const* pubkey, char const* keysub)
{
    pubnub_t* pbp = pubnub_alloc();
    if (pbp != NULL) {
        pubnub_init(pbp, pubkey, keysub);
        pubnub_set_user_id(pbp, "test_id");
        if (srand_from_pubnub_time(pbp) != 0) {
            PUBNUB_LOG_ERROR("Error :could not 'srand()' from PubNub time.\n");
        }
        pubnub_free(pbp);
    }
}


static int elapsed_ms(struct timespec prev_timspec, struct timespec timspec)
{
    int s_diff   = timspec.tv_sec - prev_timspec.tv_sec;
    int m_s_diff = (timspec.tv_nsec - prev_timspec.tv_nsec) / MILLI_IN_NANO;
    return (s_diff * UNIT_IN_MILLI) + m_s_diff;
}


static bool is_travis_pull_request_build(void)
{
    char const* tprb = getenv("TRAVIS_PULL_REQUEST");
    return (tprb != NULL) && (0 != strcmp(tprb, "false"));
}


static int run_tests(struct TestData aTest[],
                     unsigned        test_count,
                     unsigned        max_conc_thread,
                     char const*     pubkey,
                     char const*     keysub,
                     char const*     origin)
{
    unsigned next_test    = 0;
    unsigned failed_count = 0;
    unsigned passed_count = 0;
    unsigned indete_count = 0;
    struct timespec prev_timspec[TEST_COUNT];
    struct PNTestParameters tstpar = { pubkey, keysub, origin };

    tstpar.candochangroup = !is_travis_pull_request_build();
    pnfntst_set_params(&tstpar);

    printf("Starting Run of %d tests\n", test_count);
    srand_from_pubnub(pubkey, keysub);
    while (next_test < test_count) {
        unsigned i;
        unsigned in_this_pass = max_conc_thread;
        if (next_test + in_this_pass > test_count) {
            in_this_pass = test_count - next_test;
        }
        for (i = next_test; i < next_test + in_this_pass; ++i) {
            printf("Creating a thread for test %u\n", i + 1);
            monotonic_clock_get_time(&prev_timspec[i]);
            if (0 != pthread_create(&aTest[i].pth, NULL, aTest[i].pf, &aTest[i].result)) {
                printf(
                    "Failed to create a thread for test %u ('%s'), errno=%d\n",
                    i + 1,
                    aTest[i].name,
                    errno);
            }
        }
        /* This is the simplest way to do it - join all threads, one
           by one.  We could improve this, wait for the first thread
           that finishes, and start a new test "in its place", but
           that requires more work, as there is no
           pthread_join_any()...
         */
        for (i = next_test; i < next_test + in_this_pass; ++i) {
            struct timespec timspec;
            if (0 != pthread_join(aTest[i].pth, NULL)) {
                printf("Failed to join thread for test %u ('%s'), errno=%d\n",
                       i + 1,
                       aTest[i].name,
                       errno);
            }
            monotonic_clock_get_time(&timspec);
            PUBNUB_LOG_TRACE("%d.test lasted %d milliseconds.\n",
                             i+1,
                             elapsed_ms(prev_timspec[i], timspec));
            switch (aTest[i].result) {
            case trFail:
                printf(
                    "\n\x1b[41m !!!!!!! The %u. test ('%s') failed!\x1b[m\n\n",
                    i + 1,
                    aTest[i].name);
                ++failed_count;
                break;
            case trPass:
                ++passed_count;
                break;
            case trIndeterminate:
                ++indete_count;
                printf("\x1b[33m Indeterminate %u. test ('%s') of %u\x1b[m\t",
                       i + 1,
                       aTest[i].name,
                       test_count);
                /* Should restart the test... */
                // printf("\x1b[33m ReStarting %d. test of %ld\x1b[m\t", i + 1,
                // test_count);
                break;
            }
        }
        next_test = i;
    }

    puts("Test run over.");
    if (passed_count == test_count) {
        printf("\x1b[32m All %d tests passed\x1b[m\n", test_count);
        return 0;
    }
    else {
        printf("\x1b[32m %u tests passed\x1b[m, \x1b[41m %u tests "
               "failed!\x1b[m, \x1b[33m %u tests indeterminate\x1b[m\n",
               passed_count,
               failed_count,
               indete_count);
        for (next_test = 0; next_test < test_count; ++next_test) {
            switch (aTest[next_test].result) {
            case trFail:
                printf("Test \x1b[41m '%s' \x1b[m failed!\n", aTest[next_test].name);
                break;
            case trIndeterminate:
                printf("Test \x1b[33m '%s' \x1b[m indeterminate\n",
                       aTest[next_test].name);
                break;
            default:
                break;
            }
        }
        return failed_count;
    }
}


static char const* getenv_ex(char const* env, char const* dflt)
{
    char const* s = getenv(env);
    return (NULL == s) ? dflt : strdup(s);
}


int main(int argc, char* argv[])
{
    char const* pubkey = getenv_ex("PUBNUB_PUBKEY", (argc > 1) ? argv[1] : "demo");
    char const* keysub = getenv_ex("PUBNUB_KEYSUB", (argc > 2) ? argv[2] : "demo");
    char const* origin =
        getenv_ex("PUBNUB_ORIGIN", (argc > 3) ? argv[3] : "pubsub.pubnub.com");
    unsigned max_conc_thread = (argc > 4) ? atoi(argv[4]) : 1;

    return run_tests(m_aTest, TEST_COUNT, max_conc_thread, pubkey, keysub, origin);
}
