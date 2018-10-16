/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_FNTEST_PAL
#define INC_PUBNUB_FNTEST_PAL

#include "core/pubnub_helper.h"

#include <pthread.h>

#include <stdio.h>


#define TEST_BEGIN()

#define TEST_DEFER(f, d) pthread_cleanup_push((f), (d))

#define TEST_POP_DEFERRED pthread_cleanup_pop(1)

#define TEST_DECL(tst) void* pnfn_test_##tst(void* pResult);

#define TEST_DEF(tst)                                                          \
    void* pnfn_test_##tst(void* pResult)                                       \
    {                                                                          \
        char const* const this_test_name_ = #tst;                              \
        TEST_BEGIN();

#define TEST_END()                                                             \
    *(enum PNFNTestResult*)pResult = trPass;                                   \
    return NULL

#define TEST_ENDDEF                                                            \
    TEST_END();                                                                \
    }

#define TEST_YIELD() sched_yield()

#define TEST_SLEEP_FOR(ms)                                                     \
    do {                                                                       \
        struct timespec req_ = { 0, 0 };                                       \
        time_t          sec_ = (int)((ms) / 1000);                             \
        req_.tv_sec          = sec_;                                           \
        req_.tv_nsec         = ((ms) - (sec_ * 1000)) * 1000000L;              \
        while (nanosleep(&req_, &req_) == -1)                                  \
            continue;                                                          \
    } while (0)

#define TEST_INDETERMINATE                                                     \
    do {                                                                       \
        *(enum PNFNTestResult*)pResult = trIndeterminate;                      \
        pthread_exit(NULL);                                                    \
    } while (0)

#define expect(exp)                                                            \
    if (exp) {                                                                 \
    }                                                                          \
    else {                                                                     \
        printf("\x1b[33m expect(%s) failed, file %s function %s line "         \
               "%d\x1b[0m\n",                                                  \
               #exp,                                                           \
               __FILE__,                                                       \
               __FUNCTION__,                                                   \
               __LINE__);                                                      \
        *(enum PNFNTestResult*)pResult = trFail;                               \
        pthread_exit(NULL);                                                    \
    }

#define expect_pnr(rslt, exp_rslt)                                             \
    if ((rslt) != (exp_rslt)) {                                                \
        printf("\x1b[33m Expected result %d (%s) but got %d (%s), file %s "    \
               "function %s line %d\x1b[0m\n",                                 \
               (exp_rslt),                                                     \
               #exp_rslt,                                                      \
               (rslt),                                                         \
               pubnub_res_2_string(rslt),                                      \
               __FILE__,                                                       \
               __FUNCTION__,                                                   \
               __LINE__);                                                      \
        *(enum PNFNTestResult*)pResult = trFail;                               \
        pthread_exit(NULL);                                                    \
    }

#define expect_pnr_maybe_started(rslt, pbp, time_ms, exp_rslt)                 \
    if ((rslt) == (PNR_STARTED)) {                                             \
        await_timed((time_ms), (exp_rslt), (pbp));                             \
    }                                                                          \
    else {                                                                     \
        expect_last_result((pbp), (rslt), (exp_rslt));                         \
    }


#define pbpub_outof_quota(pbp, rslt)                                           \
    (((rslt) == PNR_PUBLISH_FAILED)                                            \
     && (PNPUB_ACCOUNT_QUOTA_EXCEEDED                                          \
         == pubnub_parse_publish_result(pubnub_last_publish_result(pbp))))


#define expect_last_result(pbp, rslt, exp_rslt)                                \
    if ((rslt) == (exp_rslt)) {                                                \
    }                                                                          \
    else if (((rslt) == PNR_ABORTED) || pbpub_outof_quota(pbp, rslt)) {        \
        *(enum PNFNTestResult*)pResult = trIndeterminate;                      \
        pthread_exit(NULL);                                                    \
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
        pthread_cleanup_pop(1);                                                \
    } while (0)


#define await_w_timer_2(rslt1, rslt2, tmr, pbp1, exp_rslt1, pbp2, exp_rslt2)   \
    do {                                                                       \
        enum pubnub_res M_rslt_1 = rslt1;                                      \
        enum pubnub_res M_rslt_2 = rslt2;                                      \
        while (pnfntst_timer_is_running(tmr)) {                                \
            if (M_rslt_1 == PNR_STARTED) {                                     \
                M_rslt_1 = pubnub_last_result(pbp1);                           \
            }                                                                  \
            if (M_rslt_2 == PNR_STARTED) {                                     \
                M_rslt_2 = pubnub_last_result(pbp2);                           \
            }                                                                  \
            if ((PNR_STARTED != M_rslt_1) && (PNR_STARTED != M_rslt_2)) {      \
                expect_last_result(pbp1, M_rslt_1, (exp_rslt1));               \
                expect_last_result(pbp2, M_rslt_2, (exp_rslt2));               \
                break;                                                         \
            }                                                                  \
        }                                                                      \
        expect(pnfntst_timer_is_running(tmr));                                 \
    } while (0)


#define await_timed_2(ms, exp_rslt1, pbp1, exp_rslt2, pbp2)                            \
    do {                                                                               \
        pnfntst_timer_t* M_t_ = pnfntst_alloc_timer();                                 \
        expect(M_t_ != NULL);                                                          \
        TEST_DEFER(pnfntst_free_timer, M_t_);                                          \
        pnfntst_start_timer(M_t_, (ms));                                               \
        await_w_timer_2(                                                               \
            PNR_STARTED, PNR_STARTED, M_t_, (pbp1), (exp_rslt1), (pbp2), (exp_rslt2)); \
        pthread_cleanup_pop(1);                                                        \
    } while (0)

#define expect_pnr_maybe_started_2(                                            \
    rslt1, rslt2, time_ms, pbp1, exp_rslt1, pbp2, exp_rslt2)                   \
    do {                                                                       \
        pnfntst_timer_t* M_t_ = pnfntst_alloc_timer();                         \
        expect(M_t_ != NULL);                                                  \
        TEST_DEFER(pnfntst_free_timer, M_t_);                                  \
        pnfntst_start_timer(M_t_, (time_ms));                                  \
        await_w_timer_2(                                                       \
            (rslt1), (rslt2), M_t_, (pbp1), (exp_rslt1), (pbp2), (exp_rslt2)); \
        pthread_cleanup_pop(1);                                                \
    } while (0)

#define expect_PNR_OK(pbp, trans, timeout)                                     \
    do {                                                                       \
        enum pubnub_res M_rslt_ = trans;                                       \
        expect_pnr_maybe_started(M_rslt_, pbp, timeout, PNR_OK);               \
    } while (0)

#define await_console()                                                        \
    do {                                                                       \
    } while ('\n' != getchar())


#endif /* !defined INC_PUBNUB_FNTEST_PAL */
