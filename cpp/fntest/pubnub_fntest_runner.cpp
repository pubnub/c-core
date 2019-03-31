/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_fntest_basic.hpp"
#include "pubnub_fntest_medium.hpp"

#include "core/srand_from_pubnub_time.h"
#include "core/pubnub_log.h"
#if defined _WIN32
#include "windows/console_subscribe_paint.h"
#else
#include "posix/console_subscribe_paint.h"
#endif

#include <iostream>
#include <functional>
#include <condition_variable>
#include <thread>

#include <cstdlib>
#include <cstring>


enum class TestResult {
    fail,
    pass,
    indeterminate
};

using TestFN_T = std::function<void(std::string const&,
                                    std::string const&,
                                    std::string const&,
                                    bool const&)>;

struct TestData {
    TestFN_T pf;
    char const *name;
    TestResult result;
};

#define LIST_TEST(tstname) { pnfn_test_##tstname, #tstname, TestResult::indeterminate }

static TestData m_aTest[] = {
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
/*
	LIST_TEST(broken_connection_test),
	LIST_TEST(broken_connection_test_multi),
	LIST_TEST(broken_connection_test_group),
	LIST_TEST(broken_connection_test_multi_in_group),
	LIST_TEST(broken_connection_test_group_in_group_out),
	LIST_TEST(broken_connection_test_group_multichannel_out),
 */
    LIST_TEST(complex_send_and_receive_over_several_channels_simultaneously),
    LIST_TEST(complex_send_and_receive_over_channel_plus_group_simultaneously),
    LIST_TEST(connect_disconnect_and_connect_again),
    LIST_TEST(connect_disconnect_and_connect_again_group),
    LIST_TEST(connect_disconnect_and_connect_again_combo),
    LIST_TEST(wrong_api_usage),
    LIST_TEST(handling_errors_from_pubnub)
};

#define TEST_COUNT (sizeof m_aTest / sizeof m_aTest[0])


/// Mutex to be used with #m_cndvar
static std::mutex m_mtx;

/// Condition variable to guard the #m_running tests
static std::condition_variable m_cndvar;

/// The number of currently running tests. When it goes to
/// 0, it's time for another round of tests.
static unsigned m_running_tests;

static bool is_pull_request_build(void)
{
#if !defined _WIN32
    char const* tprb = getenv("TRAVIS_PULL_REQUEST");
    return (tprb != NULL) && (0 != strcmp(tprb, "false"));
#else
    return NULL != getenv("APPVEYOR_PULL_REQUEST_NUMBER");
#endif
}

static void srand_from_pubnub(char const* pubkey, char const* keysub)
{
    pubnub_t* pbp = pubnub_alloc();
    if (pbp != NULL) {
        pubnub_init(pbp, pubkey, keysub);
        if (srand_from_pubnub_time(pbp) != 0) {
            PUBNUB_LOG_ERROR("Error :could not srand from PubNub time.\n");
        }
        pubnub_free(pbp);
    }
}

static void notify(TestData &test,TestResult result)
{
    {
        std::lock_guard<std::mutex>  lk(m_mtx);
        --m_running_tests;
        test.result = result;
    }
    m_cndvar.notify_one();
}
/// The "real main" function to run all the tests.  Each test will run
/// in its own thread, so that they can run in parallel, if we want
/// them to.
static int run_tests(TestData aTest[], unsigned test_count, unsigned max_conc_thread, std::string& pubkey, std::string& keysub, std::string& origin)
{
    unsigned next_test = 0;
    std::vector<unsigned> failed;
    unsigned passed_count = 0;
    unsigned indete_count = 0;
    std::vector<std::thread> runners(test_count);
    bool cannot_do_chan_group;
 
    cannot_do_chan_group = is_pull_request_build();

    std::cout << "Starting Run of " << test_count << " tests" << std::endl;
    while (next_test < test_count) {
        unsigned i;
        unsigned in_this_pass = max_conc_thread;
        if (next_test + in_this_pass > test_count) {
            in_this_pass = test_count - next_test;
        }
        m_running_tests = in_this_pass;
        /// first, launch the threads, one per and for test
        for (i = next_test; i < next_test+in_this_pass; ++i) {
            runners[i-next_test] =
                std::thread([i, pubkey, keysub, origin, aTest, cannot_do_chan_group] {
                    try {
                        srand_from_pubnub(pubkey.c_str(), keysub.c_str());
                        aTest[i].pf(pubkey.c_str(),
                                    keysub.c_str(),
                                    origin.c_str(),
                                    cannot_do_chan_group);
                        notify(aTest[i], TestResult::pass);
                    }
                    catch (std::exception &ex) {
                        std::cout << std::endl;
                        paint_text_white_with_background_red();
                        std::cout << " !! " << i+1 << ". test '" << aTest[i].name << "' failed!"
                                  << std::endl << "Error description: " << ex.what();
                        reset_text_paint();
                        std::cout << std::endl << std::endl;
                        notify(aTest[i], TestResult::fail);
                    }
                    catch (pubnub::except_test &ex) {
                        std::cout << std::endl;
                        paint_text_yellow();
                        std::cout << " !! " << i+1 << ". test '" << aTest[i].name << "' indeterminate!"
                                  << std::endl << "Description: " << ex.what();
                        reset_text_paint();
                        std::cout << std::endl << std::endl;
                        notify(aTest[i], TestResult::indeterminate);
                    }
                });
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        /// Await for them all to finish
        {
            std::unique_lock<std::mutex> lk(m_mtx);
             m_cndvar.wait(lk, []{ return m_running_tests == 0; });
        }
        /// Now get all of their results and process them
        for (i = next_test; i < next_test+in_this_pass; ++i) {
            runners[i-next_test].join();
            switch (aTest[i].result) {
            case TestResult::fail:
                failed.push_back(i);
                break;
            case TestResult::pass:
                ++passed_count;
                break;
            case TestResult::indeterminate:
                ++indete_count;
                /* Should restart the test... */
                break;
            }
        }
        next_test = i;
    }
    paint_text_white();
    std::cout << "\nTest run over." << std::endl;
    if (passed_count == test_count) {
        paint_text_green();
        std::cout << "All " << test_count << " tests passed" << std::endl;
        paint_text_white();
        return 0;
    }
    else {
        paint_text_green();
        std::cout << passed_count << " tests passed, ";
        reset_text_paint();
        paint_text_white_with_background_red();
        std::cout << failed.size() << " tests failed!";
        reset_text_paint();
        paint_text_white();
        std::cout << ", ";
        paint_text_yellow();
        std::cout << indete_count << " tests indeterminate" << std::endl; 
        reset_text_paint();
        if (!failed.empty()) {
            unsigned i;
            paint_text_white_with_background_red();
            std::cout << "Failed tests:\n";
            for (i = 0; i < failed.size() - 1 ; ++i) {
                std::cout << failed[i]+1 << ". " << aTest[failed[i]].name << std::endl;
            }
            std::cout << failed[i]+1 << ". " << aTest[failed[i]].name;
            reset_text_paint();
            std::cout << std::endl;
        }
        paint_text_white();
        return failed.size();
    }
}


std::string getenv_ex(char const *env, char const *dflt)
{
    char const* s = getenv(env);
    return (NULL == s) ? dflt : s;
}


int main(int argc, char *argv[])
{
    std::string pubkey = getenv_ex("PUBNUB_PUBKEY", (argc > 1) ? argv[1] : "demo");
    std::string keysub = getenv_ex("PUBNUB_KEYSUB", (argc > 2) ? argv[2] : "demo");
    std::string origin = getenv_ex("PUBNUB_ORIGIN", (argc > 3) ? argv[3] : "pubsub.pubnub.com");
    unsigned max_conc_thread = (argc > 4) ? std::atoi(argv[4]) : 1;

    return run_tests(m_aTest, TEST_COUNT, max_conc_thread, pubkey, keysub, origin);
}
