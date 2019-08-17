/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_V2_MESSAGE_HPP
#define INC_PUBNUB_V2_MESSAGE_HPP

#if !PUBNUB_USE_SUBSCRIBE_V2
#error To use the subscribe V2 API you must define PUBNUB_USE_SUBSCRIBE_V2=1
#endif

#include <cstring>

namespace pubnub {

class v2_message {
    struct pubnub_v2_message d_;

public:
    v2_message(struct pubnub_v2_message message_v2) { d_ = message_v2; }
    v2_message() { memset(&d_, 0, sizeof d_); }
    std::string tt() const { return std::string(d_.tt.ptr, d_.tt.size); }
    int region() const { return d_.region; }
    int flags() const { return d_.flags; }
    std::string channel() const { return std::string(d_.channel.ptr, d_.channel.size); } 
    std::string match_or_group() const
    {
        return std::string(d_.match_or_group.ptr, d_.match_or_group.size);
    } 
    std::string payload() const { return std::string(d_.payload.ptr, d_.payload.size); } 
    std::string metadata() const { return std::string(d_.metadata.ptr, d_.metadata.size); } 
    pubnub_message_type message_type() const { return d_.message_type; }
    /** Message structure is considered empty if there is no timetoken info(whose
        existence is obligatory for any valid v2 message)
      */
    bool is_empty() const { return (0 == d_.tt.ptr) || (0 == d_.tt.size); }
};

} // namespace pubnub

#endif /* INC_PUBNUB_V2_MESSAGE_HPP */
