/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PBNTF_TRANS_OUTCOME_COMMON
#define      INC_PBNTF_TRANS_OUTCOME_COMMON

/** This macro does the common "stuff to do" on the outcome of a
    transaction. Should be used by all `pbntf_trans_outcome()`
    functions.

    In case of PubNub protocol error, resets the time-token. This
    means some messages were (possibly) lost, but allows us to recover
    from bad situations, e.g. too many messages queued or unexpected
    problem caused by a particular message.
*/
#define PBNTF_TRANS_OUTCOME_COMMON(pb, state)                                      \
    do {                                                                           \
        pubnub_t*       M_pb_     = (pb);                                          \
        enum pubnub_res M_pbrslt_ = M_pb_->core.last_result;                       \
        PUBNUB_LOG_INFO("Context %p Transaction outcome: %d\n", M_pb_, M_pbrslt_); \
        switch (M_pbrslt_) {                                                       \
        case PNR_FORMAT_ERROR:                                                     \
        case PNR_SUB_TT_FORMAT_ERROR:                                                \
        case PNR_SUB_NO_TT_ERROR:                                                    \
        case PNR_SUB_NO_REG_ERROR:                                                   \
            PUBNUB_LOG_WARNING("Context %p Resetting time token\n", M_pb_);        \
            M_pb_->core.timetoken[0] = '0';                                        \
            M_pb_->core.timetoken[1] = '\0';                                       \
            break;                                                                 \
        default:                                                                   \
            break;                                                                 \
        }                                                                          \
        M_pb_->method = pubnubSendViaGET;                                          \
        M_pb_->state = state;                                                      \
    } while (0)

#endif /* INC_PBNTF_TRANS_OUTCOME_COMMON */
