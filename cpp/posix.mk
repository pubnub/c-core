SOURCEFILES = ../core/pubnub_pubsubapi.c ../core/pubnub_coreapi.c ../core/pubnub_coreapi_ex.c ../core/pubnub_ccore_pubsub.c ../core/pubnub_ccore.c ../core/pubnub_netcore.c  ../lib/sockets/pbpal_sockets.c ../lib/sockets/pbpal_resolv_and_connect_sockets.c ../lib/sockets/pbpal_handle_socket_error.c ../core/pubnub_alloc_std.c ../core/pubnub_assert_std.c ../core/pubnub_generate_uuid.c ../core/pubnub_blocking_io.c ../posix/posix_socket_blocking_io.c ../core/pubnub_timers.c ../core/pubnub_json_parse.c ../lib/md5/md5.c ../lib/base64/pbbase64.c ../lib/pb_strnlen_s.c ../core/pubnub_helper.c pubnub_version_posix.cpp ../posix/pubnub_generate_uuid_posix.c ../posix/pbpal_posix_blocking_io.c ../core/pubnub_free_with_timeout_std.c pubnub_subloop.cpp ../posix/msstopwatch_monotonic_clock.c ../posix/pbtimespec_elapsed_ms.c ../core/pubnub_url_encode.c ../core/pubnub_memory_block.c ../posix/pb_sleep_ms.c

ifndef ONLY_PUBSUB_API
ONLY_PUBSUB_API = 0
endif

ifndef USE_PROXY
USE_PROXY = 1
endif

ifndef USE_GZIP_COMPRESSION
USE_GZIP_COMPRESSION = 1
endif

ifndef RECEIVE_GZIP_RESPONSE
RECEIVE_GZIP_RESPONSE = 1
endif

ifndef USE_SUBSCRIBE_V2
USE_SUBSCRIBE_V2 = 1
endif

ifndef USE_ADVANCED_HISTORY
USE_ADVANCED_HISTORY = 1
endif

ifndef USE_OBJECTS_API
USE_OBJECTS_API = 1
endif

ifndef USE_ACTIONS_API
USE_ACTIONS_API = 1
endif

ifndef USE_AUTO_HEARTBEAT
USE_AUTO_HEARTBEAT = 1
endif

ifndef USE_REVOKE_TOKEN
USE_REVOKE_TOKEN = 0
endif

ifndef USE_GRANT_TOKEN
USE_GRANT_TOKEN = 0
endif

ifndef USE_FETCH_HISTORY
USE_FETCH_HISTORY = 1
endif

ifeq ($(USE_PROXY), 1)
SOURCEFILES += ../core/pubnub_proxy.c ../core/pubnub_proxy_core.c ../core/pbhttp_digest.c ../core/pbntlm_core.c ../core/pbntlm_packer_std.c
endif

ifeq ($(USE_GZIP_COMPRESSION), 1)
SOURCEFILES += ../lib/miniz/miniz_tdef.c ../lib/miniz/miniz.c ../lib/pbcrc32.c ../core/pbgzip_compress.c
endif

ifeq ($(RECEIVE_GZIP_RESPONSE), 1)
SOURCEFILES += ../lib/miniz/miniz_tinfl.c ../core/pbgzip_decompress.c
endif

ifeq ($(USE_SUBSCRIBE_V2), 1)
SOURCEFILES += ../core/pbcc_subscribe_v2.c ../core/pubnub_subscribe_v2.c 
endif

ifeq ($(USE_ADVANCED_HISTORY), 1)
SOURCEFILES += ../core/pbcc_advanced_history.c ../core/pubnub_advanced_history.c
endif

ifeq ($(USE_OBJECTS_API), 1)
SOURCEFILES += ../core/pbcc_objects_api.c ../core/pubnub_objects_api.c
endif

ifeq ($(USE_ACTIONS_API), 1)
SOURCEFILES += ../core/pbcc_actions_api.c ../core/pubnub_actions_api.c
endif

ifeq ($(USE_AUTO_HEARTBEAT), 1)
SOURCEFILES += ../core/pbauto_heartbeat.c ../posix/pbauto_heartbeat_init_posix.c ../lib/pbstr_remove_from_list.c
endif

ifeq ($(USE_FETCH_HISTORY), 1)
SOURCEFILES += ../core/pubnub_fetch_history.c ../core/pbcc_fetch_history.c  
endif

OS := $(shell uname)
ifeq ($(OS),Darwin)
SOURCEFILES += ../posix/monotonic_clock_get_time_darwin.c
LDLIBS=-lpthread
else
SOURCEFILES += ../posix/monotonic_clock_get_time_posix.c
LDLIBS=-lrt -lpthread
endif

CFLAGS =-g -I .. -I ../posix -I . -Wall -D PUBNUB_THREADSAFE -D PUBNUB_LOG_LEVEL=PUBNUB_LOG_LEVEL_WARNING -D PUBNUB_ONLY_PUBSUB_API=$(ONLY_PUBSUB_API) -D PUBNUB_PROXY_API=$(USE_PROXY) -D PUBNUB_USE_GZIP_COMPRESSION=$(USE_GZIP_COMPRESSION) -D PUBNUB_RECEIVE_GZIP_RESPONSE=$(RECEIVE_GZIP_RESPONSE) -D PUBNUB_USE_SUBSCRIBE_V2=$(USE_SUBSCRIBE_V2) -D PUBNUB_USE_OBJECTS_API=$(USE_OBJECTS_API) -D PUBNUB_USE_ACTIONS_API=$(USE_ACTIONS_API) -D PUBNUB_USE_AUTO_HEARTBEAT=$(USE_AUTO_HEARTBEAT) -D PUBNUB_USE_GRANT_TOKEN_API=$(USE_GRANT_TOKEN) -D PUBNUB_USE_REVOKE_TOKEN_API=$(USE_REVOKE_TOKEN) -D PUBNUB_USE_FETCH_HISTORY=$(USE_FETCH_HISTORY)
# -g enables debugging, remove to get a smaller executable


all: cpp11 cpp98

cpp98: pubnub_sync_sample pubnub_callback_sample cancel_subscribe_sync_sample subscribe_publish_callback_sample futres_nesting_sync futres_nesting_callback pubnub_sync_subloop_sample pubnub_callback_subloop_sample

cpp11: pubnub_callback_cpp11_sample futres_nesting_callback_cpp11 fntest_runner pubnub_callback_cpp11_subloop_sample

SYNC_INTF_SOURCEFILES=../core/pubnub_ntf_sync.c ../core/srand_from_pubnub_time.c

pubnub_sync_sample: samples/pubnub_sample.cpp $(SOURCEFILES) $(SYNC_INTF_SOURCEFILES) pubnub_futres_sync.cpp
	$(CXX) -o $@ $(CFLAGS)  -x c++ samples/pubnub_sample.cpp $(SYNC_INTF_SOURCEFILES) pubnub_futres_sync.cpp $(SOURCEFILES) $(LDLIBS)

cancel_subscribe_sync_sample: samples/cancel_subscribe_sync_sample.cpp $(SOURCEFILES) $(SYNC_INTF_SOURCEFILES) pubnub_futres_sync.cpp
	$(CXX) -o $@ $(CFLAGS)  -x c++ samples/cancel_subscribe_sync_sample.cpp $(SYNC_INTF_SOURCEFILES) pubnub_futres_sync.cpp $(SOURCEFILES) $(LDLIBS)

futres_nesting_sync: samples/futres_nesting.cpp $(SOURCEFILES) $(SYNC_INTF_SOURCEFILES) pubnub_futres_sync.cpp
	$(CXX) -o $@ $(CFLAGS)  -x c++ samples/futres_nesting.cpp $(SYNC_INTF_SOURCEFILES) pubnub_futres_sync.cpp $(SOURCEFILES) $(LDLIBS)

pubnub_sync_subloop_sample: samples/pubnub_subloop_sample.cpp $(SOURCEFILES) $(SYNC_INTF_SOURCEFILES) pubnub_futres_sync.cpp
	$(CXX) -o $@ $(CFLAGS)  -x c++ samples/pubnub_subloop_sample.cpp $(SYNC_INTF_SOURCEFILES) pubnub_futres_sync.cpp $(SOURCEFILES) $(LDLIBS)

##
# The socket poller module to use. You should use the `poll` poller, it
# doesn't have the weird restrictions of `select` poller. OTOH,
# select() on Windows is compatible w/BSD sockets select(), while
# WSAPoll() has some weird differences to poll().  The names are the
# same until the last `_`, then it's `poll` vs `select`.
SOCKET_POLLER_C=../lib/sockets/pbpal_ntf_callback_poller_poll.c

CALLBACK_INTF_SOURCEFILES= ../posix/pubnub_ntf_callback_posix.c ../posix/pubnub_get_native_socket.c ../core/pubnub_timer_list.c ../lib/sockets/pbpal_adns_sockets.c ../lib/pubnub_dns_codec.c $(SOCKET_POLLER_C)  ../core/pbpal_ntf_callback_queue.c ../core/pbpal_ntf_callback_admin.c ../core/pbpal_ntf_callback_handle_timer_list.c  ../core/pubnub_callback_subscribe_loop.c

ifndef USE_DNS_SERVERS
USE_DNS_SERVERS = 1
endif

ifndef USE_IPV6
USE_IPV6 = 1
endif

ifeq ($(USE_DNS_SERVERS), 1)
CALLBACK_INTF_SOURCEFILES += ../core/pubnub_dns_servers.c ../posix/pubnub_dns_system_servers.c ../lib/pubnub_parse_ipv4_addr.c
endif

ifeq ($(USE_IPV6), 1)
CALLBACK_INTF_SOURCEFILES += ../lib/pubnub_parse_ipv6_addr.c
endif

CFLAGS_CALLBACK = -D PUBNUB_USE_IPV6=$(USE_IPV6) -D PUBNUB_SET_DNS_SERVERS=$(USE_DNS_SERVERS)

pubnub_callback_sample: samples/pubnub_sample.cpp $(SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_posix.cpp
	$(CXX) -o $@ -D PUBNUB_CALLBACK_API $(CFLAGS) $(CFLAGS_CALLBACK) -D PUBNUB_LOG_LEVEL=PUBNUB_LOG_LEVEL_WARNING  -x c++ samples/pubnub_sample.cpp $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_posix.cpp $(SOURCEFILES) $(LDLIBS) $(LDLIBS)

pubnub_callback_cpp11_sample: samples/pubnub_sample.cpp $(SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_cpp11.cpp
	$(CXX) -o $@ -std=c++11 -D PUBNUB_CALLBACK_API $(CFLAGS) $(CFLAGS_CALLBACK) -D PUBNUB_LOG_LEVEL=PUBNUB_LOG_LEVEL_WARNING  -x c++ samples/pubnub_sample.cpp $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_cpp11.cpp $(SOURCEFILES) $(LDLIBS)

subscribe_publish_callback_sample: samples/subscribe_publish_callback_sample.cpp $(SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_posix.cpp
	$(CXX) -o $@ -D PUBNUB_CALLBACK_API $(CFLAGS) $(CFLAGS_CALLBACK) -x c++ samples/subscribe_publish_callback_sample.cpp $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_posix.cpp $(SOURCEFILES) $(LDLIBS)

futres_nesting_callback: samples/futres_nesting.cpp $(SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_posix.cpp
	$(CXX) -o $@ -D PUBNUB_CALLBACK_API $(CFLAGS) $(CFLAGS_CALLBACK) -x c++ samples/futres_nesting.cpp $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_posix.cpp $(SOURCEFILES) $(LDLIBS)

futres_nesting_callback_cpp11: samples/futres_nesting.cpp $(SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_cpp11.cpp
	$(CXX) -o $@ -std=c++11 -D PUBNUB_CALLBACK_API $(CFLAGS) $(CFLAGS_CALLBACK) -x c++ samples/futres_nesting.cpp $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_cpp11.cpp $(SOURCEFILES) $(LDLIBS)

pubnub_callback_subloop_sample: samples/pubnub_subloop_sample.cpp $(SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_posix.cpp
	$(CXX) -o $@ -D PUBNUB_CALLBACK_API $(CFLAGS) $(CFLAGS_CALLBACK) -x c++ samples/pubnub_subloop_sample.cpp $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_posix.cpp $(SOURCEFILES) $(LDLIBS)

pubnub_callback_cpp11_subloop_sample: samples/pubnub_subloop_sample.cpp $(SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_cpp11.cpp
	$(CXX) -o $@ -std=c++11 -D PUBNUB_CALLBACK_API $(CFLAGS) $(CFLAGS_CALLBACK) -x c++  samples/pubnub_subloop_sample.cpp $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_cpp11.cpp $(SOURCEFILES) $(LDLIBS)

fntest_runner: fntest/pubnub_fntest_runner.cpp $(SOURCEFILES) $(SYNC_INTF_SOURCEFILES) pubnub_futres_sync.cpp fntest/pubnub_fntest.cpp fntest/pubnub_fntest_basic.cpp fntest/pubnub_fntest_medium.cpp
	$(CXX) -o $@ -std=c++11 -D PUBNUB_LOG_LEVEL=PUBNUB_LOG_LEVEL_WARNING $(CFLAGS) -x c++ fntest/pubnub_fntest_runner.cpp $(SYNC_INTF_SOURCEFILES) pubnub_futres_sync.cpp fntest/pubnub_fntest.cpp fntest/pubnub_fntest_basic.cpp fntest/pubnub_fntest_medium.cpp $(SOURCEFILES) $(LDLIBS) 


clean:
	rm pubnub_sync_sample pubnub_callback_sample pubnub_callback_cpp11_sample pubnub_callback_subloop_sample pubnub_callback_cpp11_subloop_sample cancel_subscribe_sync_sample subscribe_publish_callback_sample futres_nesting_sync pubnub_sync_subloop_sample futres_nesting_callback futres_nesting_callback_cpp11 fntest_runner *.dSYM
