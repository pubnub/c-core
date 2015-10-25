/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if !defined INC_PUBNUB_FNTEST_BASIC
#define      INC_PUBNUB_FNTEST_BASIC

#include <string>


void simple_connect_and_send_over_single_channel(std::string const &pubkey, std::string const &keysub);


void connect_and_send_over_several_channels_simultaneously(std::string const &pubkey, std::string const &keysub);


void simple_connect_and_send_over_single_channel_in_group(std::string const &pubkey, std::string const &keysub);


#endif // !defined INC_PUBNUB_FNTEST_BASIC
