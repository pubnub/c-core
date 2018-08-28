/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_FNTEST_PAL
#define INC_PUBNUB_FNTEST_PAL

#include "core/pubnub_helper.h"
#include "core/pubnub_assert.h"

#include <process.h>

#include <stdio.h>
#include <windows.h>

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

#define TEST_DECL(tst) unsigned __stdcall pnfn_test_##tst(void* pResult);

#define TEST_DEF(tst)                                                          \
    unsigned __stdcall pnfn_test_##tst(void* pResult)                          \
    {                                                                          \
        char const* const this_test_name_ = #tst;                              \
        TEST_BEGIN();

#define TEST_END()                                                             \
    *(enum PNFNTestResult*)pResult = trPass;                                   \
    return 0

#define TEST_ENDDEF                                                            \
    TEST_END();                                                                \
    }

#define TEST_YIELD() Sleep(0)

#define TEST_SLEEP_FOR(ms) Sleep(ms)

#define TEST_EXIT                                                              \
    while (iDeffered > 0) {                                                    \
        TEST_POP_DEFERRED;                                                     \
    }                                                                          \
    _endthreadex(0);

#define expect(exp)                                                               \
    if (exp) {                                                                    \
    }                                                                             \
    else {                                                                        \
        HANDLE                     hstdout_ = GetStdHandle(STD_OUTPUT_HANDLE);    \
        CONSOLE_SCREEN_BUFFER_INFO csbiInfo_;                                     \
        WORD                       wOldColorAttrs_ = FOREGROUND_INTENSITY;        \
        PUBNUB_ASSERT_OPT(hstdout_ != INVALID_HANDLE_VALUE);                      \
        if (GetConsoleScreenBufferInfo(hstdout_, &csbiInfo_)) {                   \
            wOldColorAttrs_ = csbiInfo_.wAttributes;                              \
        }                                                                         \
        SetConsoleTextAttribute(hstdout_, FOREGROUND_RED | FOREGROUND_INTENSITY); \
        printf(" expect(%s) failed, file %s function %s line %d\n",               \
               #exp,                                                              \
               __FILE__,                                                          \
               __FUNCTION__,                                                      \
               __LINE__);                                                         \
        SetConsoleTextAttribute(hstdout_, wOldColorAttrs_);                       \
        *(enum PNFNTestResult*)pResult = trFail;                                  \
        TEST_EXIT;                                                                \
    }

#define expect_pnr(rslt, exp_rslt)                                                \
    if ((rslt) != (exp_rslt)) {                                                   \
        HANDLE                     hstdout_ = GetStdHandle(STD_OUTPUT_HANDLE);    \
        CONSOLE_SCREEN_BUFFER_INFO csbiInfo_;                                     \
        WORD                       wOldColorAttrs_ = FOREGROUND_INTENSITY;        \
        PUBNUB_ASSERT_OPT(hstdout_ != INVALID_HANDLE_VALUE);                      \
        if (GetConsoleScreenBufferInfo(hstdout_, &csbiInfo_)) {                   \
            wOldColorAttrs_ = csbiInfo_.wAttributes;                              \
        }                                                                         \
        SetConsoleTextAttribute(hstdout_, FOREGROUND_RED | FOREGROUND_INTENSITY); \
        printf(" Expected result %d (%s) but got %d (%s), file %s function "      \
               "%s line %d\n",                                                    \
               (exp_rslt),                                                        \
               #exp_rslt,                                                         \
               (rslt),                                                            \
               pubnub_res_2_string(rslt),                                         \
               __FILE__,                                                          \
               __FUNCTION__,                                                      \
               __LINE__);                                                         \
        SetConsoleTextAttribute(hstdout_, wOldColorAttrs_);                       \
        *(enum PNFNTestResult*)pResult = trFail;                                  \
        TEST_EXIT;                                                                \
    }

#define expect_pnr_maybe_started(rslt, pbp, time_ms, exp_rslt)                 \
    if ((rslt) == (PNR_STARTED)) {                                             \
        await_timed((time_ms), (exp_rslt), (pbp));                             \
    }                                                                          \
    else {                                                                     \
        expect_pnr((rslt), (exp_rslt));                                        \
    }

#define expect_last_result(rslt, exp_rslt)                                     \
    {                                                                          \
        if ((rslt) == (exp_rslt)) {                                            \
        }                                                                      \
        else if ((rslt) == PNR_ABORTED) {                                      \
            *(enum PNFNTestResult*)pResult = trIndeterminate;                  \
            TEST_EXIT;                                                         \
        }                                                                      \
        else {                                                                 \
            expect_pnr((rslt), exp_rslt);                                      \
        }                                                                      \
    }


#define await_w_timer(tmr, rslt, pbp)                                          \
    while (pnfntst_timer_is_running(tmr)) {                                    \
        enum pubnub_res M_pbres_ = pubnub_last_result(pbp);                    \
        if (M_pbres_ != PNR_STARTED) {                                         \
            expect_last_result(M_pbres_, (rslt));                              \
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


#define await_w_timer_2(rslt, rslt_2, tmr, pbp, exp_rslt, pbp_2, exp_rslt_2)   \
    do {                                                                       \
        enum pubnub_res M_rslt   = rslt;                                       \
        enum pubnub_res M_rslt_2 = rslt_2;                                     \
        while (pnfntst_timer_is_running(tmr)) {                                \
            if (M_rslt == PNR_STARTED) {                                       \
                M_rslt = pubnub_last_result(pbp);                              \
            }                                                                  \
            if (M_rslt_2 == PNR_STARTED) {                                     \
                M_rslt_2 = pubnub_last_result(pbp_2);                          \
            }                                                                  \
            if ((PNR_STARTED != M_rslt) && (PNR_STARTED != M_rslt_2)) {        \
                expect_last_result(M_rslt, (exp_rslt));                        \
                expect_last_result(M_rslt_2, (exp_rslt_2));                    \
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
        TEST_POP_DEFERRED;                                                             \
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
