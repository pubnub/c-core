/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_fntest.h"

#include "pubnub_fntest_basic.h"
#include "pubnub_fntest_medium.h"

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>


typedef unsigned  (__stdcall *PF_Test_T)(void *pResult);

struct TestData {
    PF_Test_T pf;
    char const *name;
    HANDLE pth;
    enum PNFNTestResult result;
};


#define LIST_TEST(tstname) { pnfn_test_##tstname, #tstname }


static struct TestData m_aTest[] =  {
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


static int run_tests(struct TestData aTest[], unsigned test_count, unsigned max_conc_thread, char const *pubkey, char const *keysub, char const *origin)
{
    unsigned next_test = 0;
    unsigned failed_count = 0;
    unsigned passed_count = 0;
    unsigned indete_count = 0;
    HANDLE hstdout = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbiInfo; 
    WORD wOldColorAttrs = FOREGROUND_INTENSITY;

    PUBNUB_ASSERT_OPT(max_conc_thread <= TEST_MAX_HANDLES);
    PUBNUB_ASSERT_OPT(hstdout != INVALID_HANDLE_VALUE);

    g_pubkey = pubkey;
    g_keysub = keysub;
    g_origin = origin;

    printf("Starting Run of %d tests\n", test_count);
    if (GetConsoleScreenBufferInfo(hstdout, &csbiInfo)) {
        wOldColorAttrs = csbiInfo.wAttributes; 
    }

    while (next_test < test_count) {
        unsigned i;
        unsigned in_this_pass = max_conc_thread;
        HANDLE aHa[TEST_MAX_HANDLES];

        if (next_test + in_this_pass > test_count) {
            in_this_pass = test_count - next_test;
        }
        for (i = next_test; i < next_test+in_this_pass; ++i) {
            printf("Creating a thread for test %d\n", i);
            aHa[i - next_test] = aTest[i].pth = (HANDLE)_beginthreadex(NULL, 0, aTest[i].pf, &aTest[i].result, 0, NULL);
        }
        /* This is the simplest way to do it - wait for all threads to finish.
          With a little tweak, we could wait for the first that finishes and
          launch the next test (thread) "in its place". That's a TODO for
          next version.
         */
        if (WAIT_OBJECT_0 != WaitForMultipleObjects(in_this_pass, aHa, TRUE, INFINITE)) {
            SetConsoleTextAttribute(hstdout, FOREGROUND_RED | FOREGROUND_INTENSITY);
            printf("\n ! WaitForMultipleObjects failed abandonding tests!\n\n");
            SetConsoleTextAttribute(hstdout, wOldColorAttrs);
            return -1;
        }
        for (i = next_test; i < next_test+in_this_pass; ++i) {
            switch (aTest[i].result) {
	    case trFail:
                SetConsoleTextAttribute(hstdout, FOREGROUND_RED | FOREGROUND_INTENSITY);
		printf("\n!!!!!!! The %d. test ('%s') failed!\n\n", i + 1, aTest[i].name);
                ++failed_count;
                SetConsoleTextAttribute(hstdout, wOldColorAttrs);
		break;
	    case trPass:
                ++passed_count;
		break;
            case trIndeterminate:
                ++indete_count;
                SetConsoleTextAttribute(hstdout, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		printf(" Indeterminate %d. test ('%s') of %d\t", i+1, aTest[i].name, test_count);
                SetConsoleTextAttribute(hstdout, wOldColorAttrs);
                /* Should restart the test... */
		//printf("\x1b[33m ReStarting %d. test of %ld\x1b[m\t", i + 1, test_count);
                break;
            }
        }
        next_test = i;
    }
    
    puts("Test run over.");
    if (passed_count == test_count) {
        SetConsoleTextAttribute(hstdout, FOREGROUND_GREEN);
        printf(" All %d tests passed.\n", test_count);
        SetConsoleTextAttribute(hstdout, wOldColorAttrs);
        return 0;
    }
    else {
        SetConsoleTextAttribute(hstdout, FOREGROUND_GREEN);
        printf("%d tests passed, ", passed_count);
        SetConsoleTextAttribute(hstdout, FOREGROUND_RED | FOREGROUND_INTENSITY);
        printf("%d tests failed, ", failed_count);
        SetConsoleTextAttribute(hstdout, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        printf("%d tests indeterminate\n", indete_count);
        SetConsoleTextAttribute(hstdout, wOldColorAttrs);
        return failed_count + indete_count;
    }
}


int main(int argc, char *argv[])
{
    char const *pubkey = (argc > 1) ? argv[1] : "demo";
    char const *keysub = (argc > 2) ? argv[2] : "demo";
    char const *origin = (argc > 3) ? argv[3] : "pubsub.pubnub.com";
    unsigned max_conc_thread = (argc > 4) ? atoi(argv[4]) : 1;

    return run_tests(m_aTest, TEST_COUNT, max_conc_thread, pubkey, keysub, origin);
}
