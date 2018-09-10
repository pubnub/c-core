/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_FNTEST_PAL
#define INC_PUBNUB_FNTEST_PAL

#include "pubnub_helper.h"

#include <FreeRTOS.h>
#include "task.h"
#include <FreeRTOS_IP.h>


#define TEST_MAX_DEFFERED 16

#define TEST_BEGIN()                                                           \
    typedef void (*pfdeffered_t)(void*);                                       \
    pfdeffered_t aDeffered_[TEST_MAX_DEFFERED];                                \
    void*        aDeffered_pd[TEST_MAX_DEFFERED];                              \
    unsigned     iDeffered = 0;

#define TEST_DEFER(f, d)                                                       \
    PUBNUB_ASSERT_OPT(iDeffered < TEST_MAX_DEFFERED);                          \
    aDeffered_[iDeffered]   = (f);                                             \
    aDeffered_pd[iDeffered] = (d);                                             \
    ++iDeffered;

#define TEST_POP_DEFERRED                                                      \
    PUBNUB_ASSERT_OPT(iDeffered > 0);                                          \
    --iDeffered;                                                               \
    aDeffered_[iDeffered](aDeffered_pd[iDeffered]);


#define TEST_DECL(tst) enum PNFNTestResult pnfn_test_##tst(void);

#define TEST_DEF(tst)                                                          \
    enum PNFNTestREsult pnfn_test_##tst(void)                                  \
    {                                                                          \
        char const* const this_test_name_ = #tst;                              \
        TEST_BEGIN();

#define TEST_END() return trPass

#define TEST_ENDDEF                                                            \
    TEST_END();                                                                \
    }

#define TEST_YIELD() taskYIELD()

#define TEST_SLEEP_FOR(ms) vTaskDelay(pdMS_TO_TICKS(ms))

#define TEST_EXIT                                                              \
    while (iDeffered > 0) {                                                    \
        TEST_POP_DEFERRED;                                                     \
    }

#define TEST_INDETERMINATE                                                     \
    do {                                                                       \
        TEST_EXIT;                                                             \
        return trIndeterminate;                                                \
    } while (0)

#define expect(exp)                                                            \
    if (exp) {                                                                 \
    }                                                                          \
    else {                                                                     \
        FreeRTOS_printf((" expect(%s) failed, file %s function %s line %d\n",  \
                         #exp,                                                 \
                         __FILE__,                                             \
                         __FUNCTION__,                                         \
                         __LINE__));                                           \
        TEST_EXIT;                                                             \
        return trFail;                                                         \
    }

#define expect_pnr(rslt, exp_rslt)                                             \
    if ((rslt) != (exp_rslt)) {                                                \
        FreeRTOS_printf((" Expected result %d (%s) but got %d (%s), file %s "  \
                         "function %s line %d\n",                              \
                         (exp_rslt),                                           \
                         #exp_rslt,                                            \
                         (rslt),                                               \
                         pubnub_res_2_string(rslt),                            \
                         __FILE__,                                             \
                         __FUNCTION__,                                         \
                         __LINE__));                                           \
        TEST_EXIT;                                                             \
        return trFail;                                                         \
    }


#define pbpub_outof_quota(pbp, rslt)                                           \
    (((rslt) == PNR_PUBLISH_FAILED)                                            \
     && (PNPUB_ACCOUNT_QUOTA_EXCEEDED                                          \
         == pubnub_parse_publish_result(pubnub_last_publish_result(pbp))))


#define expect_last_result(pbp, rslt, exp_rslt)                                \
    if ((rslt) == (exp_rslt)) {                                                \
    }                                                                          \
    else if (((rslt) == PNR_ABORTED) || pbpub_outof_quota(pbp, rslt)) {        \
        return trIndeterminate;                                                \
    }                                                                          \
    else {                                                                     \
        expect_pnr((rslt), exp_rslt);                                          \
    }


#define await_w_timer(tmr, rslt, pbp)                                          \
    while (pnfntst_timer_is_running(tmr)) {                                    \
        enum pubnub_res M_pbres_ = pubnub_last_result(pbp);                    \
        if (M_pbres_ != PNR_STARTED) {                                         \
            expect_last_result(pbp, M_pbres_, (rslt));                         \
            break;                                                             \
        }                                                                      \
    }                                                                          \
    expect(pnfntst_timer_is_running(tmr));


#define await_timed(ms, rslt, pbp)                                             \
    do {                                                                       \
        pnfntst_timer_t* M_t_ = pnfntst_alloc_timer();                         \
        expect(M_t_ != NULL);                                                  \
        TEST_DEFER(pnfntst_free_timer, M_t_);                                  \
        pnfntst_start_timer(M_t_, (ms));                                       \
        await_w_timer(M_t_, (rslt), (pbp));                                    \
        TEST_POP_DEFERRED;                                                     \
    } while (0)


#define await_w_timer_2(tmr, rslt, pbp, rslt2, pbp2)                           \
    do {                                                                       \
        enum pubnub_res M_rslt   = PNR_STARTED;                                \
        enum pubnub_res M_rslt_2 = PNR_STARTED;                                \
        while (pnfntst_timer_is_running(tmr)) {                                \
            if (M_rslt == PNR_STARTED) {                                       \
                M_rslt = pubnub_last_result(pbp);                              \
            }                                                                  \
            if (M_rslt_2 == PNR_STARTED) {                                     \
                M_rslt_2 = pubnub_last_result(pbp2);                           \
            }                                                                  \
            if ((PNR_STARTED != M_rslt) && (PNR_STARTED != M_rslt_2)) {        \
                expect_last_result(pbp, M_rslt, (rslt));                       \
                expect_last_result(pbp2, M_rslt_2, (rslt2));                   \
                break;                                                         \
            }                                                                  \
        }                                                                      \
        expect(pnfntst_timer_is_running(tmr));                                 \
    } while (0)


#define await_timed_2(ms, rslt, pbp, rslt2, pbp2)                              \
    do {                                                                       \
        pnfntst_timer_t* M_t_ = pnfntst_alloc_timer();                         \
        expect(M_t_ != NULL);                                                  \
        TEST_DEFER(pnfntst_free_timer, M_t_);                                  \
        pnfntst_start_timer(M_t_, (ms));                                       \
        await_w_timer_2(M_t_, (rslt), (pbp), (rslt2), (pbp2));                 \
        TEST_POP_DEFERRED;                                                     \
    } while (0)


#define await_w_timer_3(tmr, rslt, pbp, rslt2, pbp2, rslt3, pbp3)              \
    do {                                                                       \
        enum pubnub_res M_rslt   = PNR_STARRTED;                               \
        enum pubnub_res M_rslt_2 = PNR_STARTED;                                \
        enum pubnub_res M_rslt_3 = PNR_STARTED;                                \
        while (pnfntst_timer_is_running(tmr)) {                                \
            if (M_rslt != PNR_STARTED) {                                       \
                M_rslt = pubnub_last_result(pbp);                              \
            }                                                                  \
            if (M_rslt_2 != PNR_STARTED) {                                     \
                M_rslt_2 = pubnub_last_result(pbp2);                           \
            }                                                                  \
            if (M_rslt_3 != PNR_STARTED) {                                     \
                M_rslt_3 = pubnub_last_result(pbp3);                           \
            }                                                                  \
            if ((PNR_STARTED != M_rslt) && (PNR_STARTED != M_rslt_2)           \
                && (PNR_STARTED != M_rslt_3)) {                                \
                expect_last_result(M_rslt, (rslt));                            \
                expect_last_result(M_rslt_2, (rslt2));                         \
                expect_last_result(M_rslt_3, (rslt3));                         \
                break;                                                         \
            }                                                                  \
        }                                                                      \
        expect(pnfntst_timer_is_running(tmr));                                 \
    } while (0)

#define await_timed_3(ms, rslt, pbp, rslt2, pbp2, rslt3, pbp3)                  \
    do {                                                                        \
        pnfntst_timer_t* M_t_ = pnfntst_alloc_timer();                          \
        expect(M_t_ != NULL);                                                   \
        TEST_DEFER(pnfntst_free_timer, M_t_);                                   \
        pnfntst_start_timer(M_t_, (ms));                                        \
        await_w_timer_3(M_t_, (rslt), (pbp), (rslt2), (pbp2), (rslt3), (pbp3)); \
        TEST_POP_DEFERRED;                                                      \
    } while (0)


#define expect_PNR_OK(pbp, trans, timeout)                                     \
    do {                                                                       \
        enum pubnub_res M_rslt_ = trans;                                       \
        expect_pnr_maybe_started(M_rslt_, pbp, timeout, PNR_OK);               \
    } while (0)

/* FreeRTOS proper doesn't have any console support.
    There is console support in FreeRTOS+CLI, but we don't want to
    have that as a requirement for running tests...
    So, the following will work on a Windows simulator, for other
    platforms either implement a replacemetn for getchar(), or change this
    macro...
    */
#include <stdio.h>
#define await_console()                                                        \
    do {                                                                       \
    } while ('\n' != getchar())


#endif /* !defined INC_PUBNUB_FNTEST_PAL */
