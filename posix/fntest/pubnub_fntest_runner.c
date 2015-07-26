/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_fntest.h"

#include "pubnub_fntest_basic.h"

#include <pthread.h>

#include <stdio.h>



typedef void* (*PF_Test_T)(void *pResult);

struct TestData {
    PF_Test_T pf;
    char const *name;
    pthread_t pth;
    enum PNFNTestResult result;
};


#define LIST_TEST(tstname) { pnfn_test_##tstname, #tstname }



static struct TestData m_aTest[] =  {
    LIST_TEST(basic_conn_pub),
    LIST_TEST(basic_conn_rx),
/*    LIST_TEST(medium_conn_pub),
    LIST_TEST(medium_conn_rx),
    LIST_TEST(medium_complex_rx_tx),
    LIST_TEST(medium_conn_disc_conn_again),
    LIST_TEST(medium_wrong_api),
    LIST_TEST(medium_cloud_err)*/
};

#define TEST_COUNT (sizeof m_aTest / sizeof m_aTest[0])


static int run_tests(struct TestData aTest[], unsigned test_count, unsigned max_conc_thread)
{
    unsigned next_test = 0;
    unsigned failed_count = 0;
    unsigned passed_count = 0;
    unsigned indete_count = 0;

    printf("Starting Run of %d tests\n", test_count);
    while (next_test < test_count) {
        unsigned i;
        unsigned in_this_pass = max_conc_thread;
        if (next_test + in_this_pass > test_count) {
            in_this_pass = test_count - next_test;
        }
        for (i = next_test; i < next_test+in_this_pass; ++i) {
            printf("Creating a thread for test %d\n", i);
            pthread_create(&aTest[i].pth, NULL, aTest[i].pf, &aTest[i].result);
        }
        /* This is the simplest way to do it - join all threads, one
           by one.  We could improve this, wait for the first thread
           that finishes, and start a new test "in its place", but
           that requires more work, as there is no
           pthread_join_any()...
         */
        for (i = next_test; i < next_test+in_this_pass; ++i) {
            pthread_join(aTest[i].pth, NULL);
            switch (aTest[i].result) {
	    case trFail:
		printf("\n\x1b[41m !!!!!!! The %d. test ('%s') failed!\x1b[m\n\n", i + 1, aTest[i].name);
                ++failed_count;
		break;
	    case trPass:
                ++passed_count;
		break;
            case trIndeterminate:
                ++indete_count;
		printf("\x1b[33m Indeterminate %d. test ('%s') of %d\x1b[m\t", i+1, aTest[i].name, test_count);
                /* Should restart the test... */
		//printf("\x1b[33m ReStarting %d. test of %ld\x1b[m\t", i + 1, test_count);
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
        printf("\x1b[32m %d tests passed\x1b[m, \x1b[41m %d tests failed!\x1b[m, \x1b[33m %d tests indeterminate\x1b[m\n", 
               passed_count,
               failed_count,
               indete_count
            );
        return failed_count + indete_count;
    }
}


int main(int argc, char *argv[])
{
    /* Should parse command-line args here */
    unsigned max_conc_thread = 1;
    return run_tests(m_aTest, TEST_COUNT, max_conc_thread);
}
