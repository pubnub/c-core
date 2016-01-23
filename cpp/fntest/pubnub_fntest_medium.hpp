/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if !defined INC_PUBNUB_FNTEST_MEDIUM
#define      INC_PUBNUB_FNTEST_MEDIUM

#include <string>

void complex_send_and_receive_over_several_channels_simultaneously(std::string const &pubkey, std::string const &keysub, std::string const &origin);

void complex_send_and_receive_over_channel_plus_group_simultaneously(std::string const &pubkey, std::string const &keysub, std::string const &origin);

void connect_disconnect_and_connect_again(std::string const &pubkey, std::string const &keysub, std::string const &origin);

void connect_disconnect_and_connect_again_group(std::string const &pubkey, std::string const &keysub, std::string const &origin);

void connect_disconnect_and_connect_again_combo(std::string const &pubkey, std::string const &keysub, std::string const &origin);

void wrong_api_usage(std::string const &pubkey, std::string const &keysub, std::string const &origin);

void handling_errors_from_pubnub(std::string const &pubkey, std::string const &keysub, std::string const &origin);

#endif // !defined INC_PUBNUB_FNTEST_MEDIUM
