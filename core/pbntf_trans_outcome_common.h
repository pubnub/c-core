/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

/** This macro does the common "stuff to do" on the outcome of a
    transaction. Should be used by all `pbntf_trans_outcome()`
    functions.

    In case of PubNub protocol error, aborts an ongoing subscribe and
    start over (by resetting the time-token). This means some messages
    were lost, but allows us to recover from bad situations, e.g. too
    many messages queued or unexpected problem caused by a particular
    message.

*/
#define PBNTF_TRANS_OUTCOME_COMMON(pb, result) do {                     \
        (pb)->core.last_result = result;                                \
        DEBUG_PRINTF("Pubnub: Transaction outcome: %d, HTTP code: %d\n", (result), (pb)->core.http_code); \
        if (((result) == PNR_FORMAT_ERROR) || (PUBNUB_MISSMSG_OK && ((result) != PNR_OK))) { \
            (pb)->core.timetoken[0] = '0';                              \
            (pb)->core.timetoken[1] = '\0';                             \
        }                                                               \
        (pb)->state = PBS_IDLE;                                          \
    } while(0)
