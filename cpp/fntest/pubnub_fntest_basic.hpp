/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if !defined INC_PUBNUB_FNTEST_BASIC
#define      INC_PUBNUB_FNTEST_BASIC

#include <string>


void simple_connect_and_send_over_single_channel(std::string const &pubkey, std::string const &keysub, std::string const &origin);


void connect_and_send_over_several_channels_simultaneously(std::string const &pubkey, std::string const &keysub, std::string const &origin);


void simple_connect_and_send_over_single_channel_in_group(std::string const &pubkey, std::string const &keysub, std::string const &origin);

void connect_and_send_over_several_channels_in_group_simultaneously(std::string const &pubkey, std::string const &keysub, std::string const &origin);

void connect_and_send_over_channel_in_group_and_single_channel_simultaneously(std::string const &pubkey, std::string const &keysub, std::string const &origin);

void connect_and_send_over_channel_in_group_and_multi_channel_simultaneously(std::string const &pubkey, std::string const &keysub, std::string const &origin);

void simple_connect_and_receiver_over_single_channel(std::string const &pubkey, std::string const &keysub, std::string const &origin);

void connect_and_receive_over_several_channels_simultaneously(std::string const &pubkey, std::string const &keysub, std::string const &origin);

void simple_connect_and_receiver_over_single_channel_in_group(std::string const &pubkey, std::string const &keysub, std::string const &origin);

void connect_and_receive_over_several_channels_in_group_simultaneously(std::string const &pubkey, std::string const &keysub, std::string const &origin);

void connect_and_receive_over_channel_in_group_and_single_channel_simultaneously(std::string const &pubkey, std::string const &keysub, std::string const &origin);

void connect_and_receive_over_channel_in_group_and_multi_channel_simultaneously(std::string const &pubkey, std::string const &keysub, std::string const &origin);

void broken_connection_test(std::string const &pubkey, std::string const &keysub, std::string const &origin);

void broken_connection_test_multi(std::string const &pubkey, std::string const &keysub, std::string const &origin);

void broken_connection_test_group(std::string const &pubkey, std::string const &keysub, std::string const &origin);

void broken_connection_test_multi_in_group(std::string const &pubkey, std::string const &keysub, std::string const &origin);

void broken_connection_test_group_in_group_out(std::string const &pubkey, std::string const &keysub, std::string const &origin);

void broken_connection_test_group_multichannel_out(std::string const &pubkey, std::string const &keysub, std::string const &origin);



#endif // !defined INC_PUBNUB_FNTEST_BASIC
