/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_fntest.hpp"
#include "pubnub_fntest_basic.hpp"
#include "pubnub_fntest_medium.hpp"

#include <iostream>
#include <functional>
#include <condition_variable>
#include <thread>

#include <cstdlib>


enum class TestResult {
    fail,
    pass,
    indeterminate
};

using TestFN_T = std::function<void(std::string const&,std::string const&, std::string const&)>;

struct TestData {
    TestFN_T pf;
    char const *name;
    TestResult result;
};


template <typename... T>
constexpr auto make_array(T&&... t) -> std::array<std::common_type<T...>, sizeof...(T)> { 
    return {{ std::forward<T>(t)... }};
}

#define LIST_TEST(tstname) { tstname, #tstname, TestResult::indeterminate }



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
    LIST_TEST(handling_errors_from_pubnub),
};

#define TEST_COUNT (sizeof m_aTest / sizeof m_aTest[0])


/// Mutex to be used with #m_cndvar
static std::mutex m_mtx;

/// Condition variable to guard the #m_running tests
static std::condition_variable m_cndvar;

/// The number of currently running tests. When it goes to
/// 0, it's time for another round of tests.
static unsigned m_running_tests;


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

    std::cout << "Starting Run of " << test_count << " tests" << std::endl;;
    while (next_test < test_count) {
        unsigned i;
        unsigned in_this_pass = max_conc_thread;
        if (next_test + in_this_pass > test_count) {
            in_this_pass = test_count - next_test;
        }
        m_running_tests = in_this_pass;
        /// first, launch the threads, one per and for test
        for (i = next_test; i < next_test+in_this_pass; ++i) {
            runners[i-next_test] = std::thread([i, pubkey, keysub, origin, aTest] {
                    try {
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                        aTest[i].pf(pubkey.c_str(), keysub.c_str(), origin.c_str());
                        {
                            std::lock_guard<std::mutex>  lk(m_mtx);
                            --m_running_tests;
                            aTest[i].result = TestResult::pass;
                        }
                        m_cndvar.notify_one();
                    }
                    catch (std::exception &ex) {
                        std::cout << "\n\x1b[41m !! " << i+1 << ". test '" << aTest[i].name << "' failed!" << std::endl << "Error description: " << ex.what() << "\x1b[m" << std::endl << std::endl;
                        {
                            std::lock_guard<std::mutex>  lk(m_mtx);
                            --m_running_tests;
                            aTest[i].result = TestResult::fail;
                        }
                        m_cndvar.notify_one();
                    }
                });
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
		printf("\x1b[33m Indeterminate %d. test ('%s') of %d\x1b[m\t", i+1, aTest[i].name, test_count);
                /* Should restart the test... */
                break;
            }
        }
        next_test = i;
    }

    std::cout << "Test run over." << std::endl;
    if (passed_count == test_count) {
        std::cout << "\x1b[32m All " << test_count << " tests passed\x1b[m" << std::endl;
        return 0;
    }
    else {
        std::cout <<"\x1b[32m " << passed_count << " tests passed\x1b[m, \x1b[41m " << failed.size() << " tests failed!\x1b[m, \x1b[33m " << indete_count << " tests indeterminate\x1b[m" << std::endl; 
        if (!failed.empty()) {
            std::cout << "Failed tests:\n";
            for (unsigned i = 0; i < failed.size(); ++i) {
                std::cout << failed[i] << ". " << aTest[failed[i]].name << '\n';
            }
        }
        return failed.size() + indete_count;
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
