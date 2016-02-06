/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

/** This macro does the common "stuff to do" on the outcome of a
    transaction. Should be used by all `pbntf_trans_outcome()`
    functions.

    In case of PubNub protocol error, resets the time-token. This means 
    some messages were (possibly) lost, but allows us to recover from bad 
    situations, e.g. too many messages queued or unexpected problem caused 
    by a particular message.
*/
#define PBNTF_TRANS_OUTCOME_COMMON(pb) do {                             \
        enum pubnub_res M_pbrslt_ = (pb)->core.last_result;             \
        PUBNUB_LOG_INFO("Pubnub Transaction outcome: %d\n", M_pbrslt_); \
        switch (M_pbrslt_) {                                            \
        case PNR_FORMAT_ERROR:                                          \
        case PNR_TIMEOUT:                                               \
        case PNR_IO_ERROR:                                              \
            (pb)->core.timetoken[0] = '0';                              \
            (pb)->core.timetoken[1] = '\0';                             \
            break;                                                      \
        default:                                                        \
            break;                                                      \
        }                                                               \
        (pb)->state = PBS_IDLE;                                         \
    } while(0)
