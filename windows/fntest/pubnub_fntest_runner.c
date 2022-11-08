/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "core/fntest/pubnub_fntest.h"

#include "core/fntest/pubnub_fntest_basic.h"
#include "core/fntest/pubnub_fntest_medium.h"

#include "core/pubnub_alloc.h"
#include "core/pubnub_pubsubapi.h"
#include "core/srand_from_pubnub_time.h"
#include "core/pubnub_log.h"

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef unsigned(__stdcall* PF_Test_T)(void* pResult);

struct TestData {
    PF_Test_T           pf;
    char const*         name;
    HANDLE              pth;
    enum PNFNTestResult result;
};


#define LIST_TEST(tstname)                                                     \
    {                                                                          \
        pnfn_test_##tstname, #tstname                                          \
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

    LIST_TEST(complex_send_and_receive_over_several_channels_simultaneously),
    LIST_TEST(complex_send_and_receive_over_channel_plus_group_simultaneously),
    LIST_TEST(connect_disconnect_and_connect_again),
    LIST_TEST(connect_disconnect_and_connect_again_group),
    LIST_TEST(connect_disconnect_and_connect_again_combo),
    LIST_TEST(wrong_api_usage),
    LIST_TEST(handling_errors_from_pubnub),

    /* "Manual-tests" (require human interaction */
    /*	LIST_TEST(broken_connection_test),
        LIST_TEST(broken_connection_test_multi),
        LIST_TEST(broken_connection_test_group),
        LIST_TEST(broken_connection_test_multi_in_group),
        LIST_TEST(broken_connection_test_group_in_group_out),
        LIST_TEST(broken_connection_test_group_multichannel_out),
    */
};

#define TEST_COUNT (sizeof m_aTest / sizeof m_aTest[0])

#define TEST_MAX_HANDLES 16


char const* g_pubkey;
char const* g_keysub;
char const* g_origin;


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


static bool is_appveyor_pull_request_build(void)
{
    return NULL != getenv("APPVEYOR_PULL_REQUEST_NUMBER");
}


static int run_tests(struct TestData aTest[],
                     unsigned        test_count,
                     unsigned        max_conc_thread,
                     char const*     pubkey,
                     char const*     keysub,
                     char const*     origin)
{
    unsigned                   next_test    = 0;
    unsigned                   failed_count = 0;
    unsigned                   passed_count = 0;
    unsigned                   indete_count = 0;
    HANDLE                     hstdout      = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
    WORD                       wOldColorAttrs = FOREGROUND_INTENSITY;
    struct PNTestParameters    tstpar         = { pubkey, keysub, origin };

    PUBNUB_ASSERT_OPT(max_conc_thread <= TEST_MAX_HANDLES);
    PUBNUB_ASSERT_OPT(hstdout != INVALID_HANDLE_VALUE);

    tstpar.candochangroup = !is_appveyor_pull_request_build();
    pnfntst_set_params(&tstpar);

    printf("Starting Run of %u tests\n", test_count);
    srand_from_pubnub(pubkey, keysub);
    if (GetConsoleScreenBufferInfo(hstdout, &csbiInfo)) {
        wOldColorAttrs = csbiInfo.wAttributes;
    }

    while (next_test < test_count) {
        unsigned i;
        unsigned in_this_pass = max_conc_thread;
        HANDLE   aHa[TEST_MAX_HANDLES];

        if (next_test + in_this_pass > test_count) {
            in_this_pass = test_count - next_test;
        }
        for (i = next_test; i < next_test + in_this_pass; ++i) {
            printf("Creating a thread for test %u\n", i + 1);
            aHa[i - next_test] = aTest[i].pth = (HANDLE)_beginthreadex(
                NULL, 0, aTest[i].pf, &aTest[i].result, 0, NULL);
        }
        /* This is the simplest way to do it - wait for all threads to finish.
          With a little tweak, we could wait for the first that finishes and
          launch the next test (thread) "in its place". That's a TODO for
          next version.
         */
        if (WAIT_OBJECT_0
            != WaitForMultipleObjects(in_this_pass, aHa, TRUE, INFINITE)) {
            SetConsoleTextAttribute(hstdout, FOREGROUND_RED | FOREGROUND_INTENSITY);
            printf("\n ! WaitForMultipleObjects failed abandonding tests!\n\n");
            SetConsoleTextAttribute(hstdout, wOldColorAttrs);
            return -1;
        }
        for (i = next_test; i < next_test + in_this_pass; ++i) {
            switch (aTest[i].result) {
            case trFail:
                SetConsoleTextAttribute(hstdout,
                                        FOREGROUND_RED | FOREGROUND_INTENSITY);
                printf(
                    "\n\x1b[41m!!!!!!! The %u. test ('%s') failed!\x1b[m\n\n",
                    i + 1,
                    aTest[i].name);
                ++failed_count;
                SetConsoleTextAttribute(hstdout, wOldColorAttrs);
                break;
            case trPass:
                ++passed_count;
                break;
            case trIndeterminate:
                ++indete_count;
                SetConsoleTextAttribute(hstdout,
                                        FOREGROUND_RED | FOREGROUND_GREEN
                                            | FOREGROUND_INTENSITY);
                printf(" Indeterminate %u. test ('%s') of %u\t",
                       i + 1,
                       aTest[i].name,
                       test_count);
                SetConsoleTextAttribute(hstdout, wOldColorAttrs);
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
        SetConsoleTextAttribute(hstdout, FOREGROUND_GREEN);
        printf(" All %u tests passed.\n", test_count);
        SetConsoleTextAttribute(hstdout, wOldColorAttrs);
        return 0;
    }
    else {
        SetConsoleTextAttribute(hstdout, FOREGROUND_GREEN);
        printf("%u tests passed, ", passed_count);
        SetConsoleTextAttribute(hstdout, FOREGROUND_RED | FOREGROUND_INTENSITY);
        printf("%u tests failed, ", failed_count);
        SetConsoleTextAttribute(
            hstdout, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        printf("%u tests indeterminate\n", indete_count);
        SetConsoleTextAttribute(hstdout, wOldColorAttrs);
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
