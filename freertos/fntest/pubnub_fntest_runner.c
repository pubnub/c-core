/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_fntest.h"

#include "pubnub_fntest_basic.h"
#include "pubnub_fntest_medium.h"

#include "pubnub_assert.h"

#include <FreeRTOS.h>
#include "queue.h"
#include "task.h"


typedef enum PNFNTestResult (*PF_Test_T)(void);

struct TestData {
    PF_Test_T pf;
    char const *name;
    TaskHandle_t task;
};


#define LIST_TEST(tstname) { pnfn_test_##tstname, #tstname, NULL }



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


char const* g_pubkey;
char const* g_keysub;
char const* g_origin;
unsigned g_max_conc_tests;


struct TestResultMessage {
    unsigned test;
    enum PNFNTestResult result;
};


static QueueHandle_t m_TestResultQueue;


void single_test_runner(void *arg)
{
    struct TestResultMessage msg;
    msg.test = (unsigned)arg;
    msg.result = m_aTest[msg.test].pf();
    xQueueSendToBack(m_TestResultQueue, &msg, portMAX_DELAY);
    for (;;) {
        vTaskDelay(100);
    }
}

#define SINGLE_TEST_STACK_DEPTH 512
#define SINGLE_TEST_PRIORITY 2


static void start_test(unsigned test)
{
    if (test < TEST_COUNT) {
        FreeRTOS_printf(("Creating a task for test %d\n", test));
        if (pdPASS != xTaskCreate(
            single_test_runner, 
            "single test", 
            SINGLE_TEST_STACK_DEPTH, 
            (void*)test, 
            SINGLE_TEST_PRIORITY, 
            &m_aTest[test].task
            )) {
            FreeRTOS_printf(("\n!! Failed to create a task for test %d!\n", test));
        }
    }
}


void test_runner(void *arg)
{
    PUBNUB_UNUSED(arg);

    for (;;) {
        unsigned next_test = 0;
        unsigned failed_count = 0;
        unsigned passed_count = 0;
        unsigned indete_count = 0;
        unsigned tests_in_progress = 0;

        FreeRTOS_printf(("Starting Run of %d tests\n", TEST_COUNT));

        while (failed_count + passed_count + indete_count < TEST_COUNT) {
            struct TestResultMessage msg;
            if ((tests_in_progress < g_max_conc_tests) && (next_test < TEST_COUNT)) {
                start_test(next_test++);
                ++tests_in_progress;
            }
            if (pdTRUE == xQueueReceive(m_TestResultQueue, &msg, pdMS_TO_TICKS(20))) {
                switch (msg.result) {
                case trFail:
                    FreeRTOS_printf(("\n !!!!!!! The %d. test ('%s') failed!\n\n", msg.test + 1, m_aTest[msg.test].name));
                    ++failed_count;
                    break;
                case trPass:
                    ++passed_count;
                    break;
                case trIndeterminate:
                    ++indete_count;
                    FreeRTOS_printf((" Indeterminate %d. test ('%s') of %d\t", msg.test+1, m_aTest[msg.test].name, TEST_COUNT));
                    /* Should restart the test... */
                    //FreeRTOS_printf((" ReStarting %d. test of %ld\t", msg.test + 1, TEST_COUNT));
                    break;
                }
#ifdef INCLUDE_vTaskDelete
                vTaskDelete(m_aTest[msg.test].task);
#endif
                --tests_in_progress;
            }
        }

        FreeRTOS_printf(("Test run over.\n"));
        if (passed_count == TEST_COUNT) {
            FreeRTOS_printf(("\n All %d tests passed\n", TEST_COUNT));
        }
        else {
            FreeRTOS_printf(("\n\n %d tests passed, %d tests failed, %d tests indeterminate\n", 
                    passed_count,
                    failed_count,
                    indete_count
                ));
        }

        vTaskDelay(pdMS_TO_TICKS(10 * 1000));
    }
}


void StartPubnubTask(uint16_t usTaskStackSize, UBaseType_t uxTaskPriority)
{
    m_TestResultQueue = xQueueCreate(g_max_conc_tests + 2, sizeof(struct TestResultMessage));
    if (0 == m_TestResultQueue) {
        FreeRTOS_printf(("\n!! Failed to create the test result queue!\n"));
    }
    if (pdPASS != xTaskCreate(
        test_runner, 
        "test runner", 
        usTaskStackSize, 
        NULL,
        uxTaskPriority, 
        NULL
        )) {
        vQueueDelete(m_TestResultQueue);
        FreeRTOS_printf(("\n!! Failed to create the task runner!\n"));
    }
}
