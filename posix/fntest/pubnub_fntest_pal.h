/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_FNTEST_PAL
#define INC_PUBNUB_FNTEST_PAL

#include "core/pubnub_helper.h"
#include "posix/monotonic_clock_get_time.h"

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
    do {                                                                       \
        enum pubnub_res M_res_ = rslt;                                         \
        if (M_res_ != (exp_rslt)) {                                            \
            printf("\x1b[33m Expected result %d (%s) but got %d (%s), file %s "\
                   "function %s line %d\x1b[0m\n",                             \
                   (exp_rslt),                                                 \
                   #exp_rslt,                                                  \
                   M_res_,                                                     \
                   pubnub_res_2_string(M_res_),                                \
                   __FILE__,                                                   \
                   __FUNCTION__,                                               \
                   __LINE__);                                                  \
            *(enum PNFNTestResult*)pResult = trFail;                           \
            pthread_exit(NULL);                                                \
        }                                                                      \
    } while (0)

#define expect_pnr_maybe_started(rslt, pbp, time_ms, exp_rslt)                 \
    if ((rslt) == (PNR_STARTED)) {                                             \
        await_timed(time_ms, exp_rslt, pbp);                                   \
    }                                                                          \
    else {                                                                     \
        expect_last_result(pbp, rslt, exp_rslt);                               \
    }


#define pbpub_outof_quota(pbp, rslt)                                           \
    ((rslt == PNR_PUBLISH_FAILED)                                              \
     && (PNPUB_ACCOUNT_QUOTA_EXCEEDED                                          \
         == pubnub_parse_publish_result(pubnub_last_publish_result(pbp))))


#define expect_last_result(pbp, rslt, exp_rslt)                                \
    if ((rslt == exp_rslt) && !pbpub_outof_quota(pbp, rslt)) {                 \
    }                                                                          \
    else if ((rslt == PNR_ABORTED) || pbpub_outof_quota(pbp, rslt)) {          \
        *(enum PNFNTestResult*)pResult = trIndeterminate;                      \
        pthread_exit(NULL);                                                    \
    }                                                                          \
    else {                                                                     \
        expect_pnr(rslt, exp_rslt);                                            \
    }


#define await_w_timer(tmr, rslt, pbp)                                          \
    do {                                                                       \
        while (pnfntst_timer_is_running(tmr)) {                                \
            rslt = pubnub_last_result(pbp);                                    \
            if (rslt != PNR_STARTED) {                                         \
                break;                                                         \
            }                                                                  \
        }                                                                      \
        expect(pnfntst_timer_is_running(tmr));                                 \
    } while (0)

#define ELAPSED_MS(prev_timspec)                                               \
    do {                                                                       \
        int M_s_diff;                                                          \
        int M_ms_diff;                                                         \
        struct timespec M_timspec;                                             \
        monotonic_clock_get_time(&M_timspec);                                  \
        M_s_diff   = M_timspec.tv_sec - (prev_timspec).tv_sec;                 \
        M_ms_diff = (M_timspec.tv_nsec - (prev_timspec).tv_nsec) / MILLI_IN_NANO;\
        PUBNUB_LOG_TRACE("-------->Extra waiting for transaction to finish took %d milliseconds.\n",\
                         (M_s_diff * UNIT_IN_MILLI) + M_ms_diff);              \
    } while (0)

#define await_timed(ms, exp_rslt, pbp)                                         \
    do {                                                                       \
        enum pubnub_res M_rslt;                                                \
        pnfntst_timer_t* M_t_ = pnfntst_alloc_timer();                         \
        struct timespec M_prev_timspec;                                        \
        expect(M_t_ != NULL);                                                  \
        TEST_DEFER(pnfntst_free_timer, M_t_);                                  \
        monotonic_clock_get_time(&M_prev_timspec);                             \
        pnfntst_start_timer(M_t_, (ms));                                       \
        await_w_timer(M_t_, M_rslt, pbp);                                      \
        PUBNUB_LOG_TRACE("pbp=%p", pbp);                                       \
        ELAPSED_MS(M_prev_timspec);                                            \
        expect_last_result(pbp, M_rslt, exp_rslt);                             \
        TEST_POP_DEFERRED;                                                     \
    } while (0)


#define await_w_timer_2(rslt1, rslt2, tmr, pbp1, pbp2)                         \
    do {                                                                       \
        while (pnfntst_timer_is_running(tmr)) {                                \
            if (rslt1 == PNR_STARTED) {                                        \
                rslt1 = pubnub_last_result(pbp1);                              \
            }                                                                  \
            if (rslt2 == PNR_STARTED) {                                        \
                rslt2 = pubnub_last_result(pbp2);                              \
            }                                                                  \
            if (((PNR_STARTED != rslt1) && (PNR_STARTED != rslt2))             \
                || pbpub_outof_quota(pbp1, rslt1)                              \
                || pbpub_outof_quota(pbp2, rslt2)) {                           \
                break;                                                         \
            }                                                                  \
        }                                                                      \
        expect(pnfntst_timer_is_running(tmr));                                 \
    } while (0)


#define expect_last_result_2(pbp1, rslt1, exp_rslt1, pbp2, rslt2, exp_rslt2)   \
    do {                                                                       \
        if (rslt1 != PNR_STARTED) {                                            \
            expect_last_result(pbp1, rslt1, exp_rslt1);                        \
        }                                                                      \
        if (rslt2 != PNR_STARTED) {                                            \
            expect_last_result(pbp2, rslt2, exp_rslt2);                        \
        }                                                                      \
    } while (0)


#define await_timed_2(ms, exp_rslt1, pbp1, exp_rslt2, pbp2)                    \
    do {                                                                       \
        enum pubnub_res M_rslt_1 = PNR_STARTED;                                \
        enum pubnub_res M_rslt_2 = PNR_STARTED;                                \
        pnfntst_timer_t* M_t_ = pnfntst_alloc_timer();                         \
        struct timespec M_prev_timspec;                                        \
        expect(M_t_ != NULL);                                                  \
        TEST_DEFER(pnfntst_free_timer, M_t_);                                  \
        monotonic_clock_get_time(&M_prev_timspec);                             \
        pnfntst_start_timer(M_t_, (ms));                                       \
        await_w_timer_2(M_rslt_1, M_rslt_2, M_t_, pbp1, pbp2);                 \
        PUBNUB_LOG_TRACE("pbp1=%p, pbp2=%p", pbp1, pbp2);                      \
        ELAPSED_MS(M_prev_timspec);                                            \
        expect_last_result_2(pbp1, M_rslt_1, exp_rslt1, pbp2, M_rslt_2, exp_rslt2);\
        TEST_POP_DEFERRED;                                                     \
    } while (0)


#define expect_pnr_maybe_started_2(                                            \
    rslt1, rslt2, time_ms, pbp1, exp_rslt1, pbp2, exp_rslt2)                   \
    do {                                                                       \
        enum pubnub_res M_rslt_1 = rslt1;                                      \
        enum pubnub_res M_rslt_2 = rslt2;                                      \
        pnfntst_timer_t* M_t_ = pnfntst_alloc_timer();                         \
        struct timespec M_prev_timspec;                                        \
        expect(M_t_ != NULL);                                                  \
        TEST_DEFER(pnfntst_free_timer, M_t_);                                  \
        monotonic_clock_get_time(&M_prev_timspec);                             \
        pnfntst_start_timer(M_t_, (time_ms));                                  \
        await_w_timer_2(M_rslt_1, M_rslt_2, M_t_, pbp1, pbp2);                 \
        PUBNUB_LOG_TRACE("pbp1=%p, pbp2=%p", pbp1, pbp2);                      \
        ELAPSED_MS(M_prev_timspec);                                            \
        expect_last_result_2(pbp1, M_rslt_1, exp_rslt1, pbp2, M_rslt_2, exp_rslt2);\
        TEST_POP_DEFERRED;                                                     \
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
