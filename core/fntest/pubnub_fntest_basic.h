/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_FNTEST_BASIC
#define	INC_PUBNUB_FNTEST_BASIC


#include "pubnub_fntest.h"

TEST_DECL(simple_connect_and_send_over_single_channel);

TEST_DECL(connect_and_send_over_several_channels_simultaneously);

TEST_DECL(simple_connect_and_send_over_single_channel_in_group);

TEST_DECL(connect_and_send_over_several_channels_in_group_simultaneously);
	
TEST_DECL(connect_and_send_over_channel_in_group_and_single_channel_simultaneously);
	
TEST_DECL(connect_and_send_over_channel_in_group_and_multi_channel_simultaneously);
	
TEST_DECL(simple_connect_and_receiver_over_single_channel);
	
TEST_DECL(connect_and_receive_over_several_channels_simultaneously);
	
TEST_DECL(simple_connect_and_receiver_over_single_channel_in_group);
	
TEST_DECL(connect_and_receive_over_several_channels_in_group_simultaneously);
	
TEST_DECL(connect_and_receive_over_channel_in_group_and_single_channel_simultaneously);
	
TEST_DECL(connect_and_receive_over_channel_in_group_and_multi_channel_simultaneously);
	
TEST_DECL(broken_connection_test);
	
TEST_DECL(broken_connection_test_multi);
		
TEST_DECL(broken_connection_test_group);
	
TEST_DECL(broken_connection_test_multi_in_group);

TEST_DECL(broken_connection_test_group_in_group_out);
		
TEST_DECL(broken_connection_test_group_multichannel_out);

#endif /* !defined INC_PUBNUB_FNTEST_BASIC */
