SOURCEFILES = ../core/pubnub_pubsubapi.c ../core/pubnub_coreapi.c ../core/pubnub_coreapi_ex.c ../core/pubnub_ccore_pubsub.c ../core/pubnub_ccore.c ../core/pubnub_netcore.c  ../lib/sockets/pbpal_sockets.c ../lib/sockets/pbpal_resolv_and_connect_sockets.c ../core/pubnub_alloc_std.c ../core/pubnub_assert_std.c ../core/pubnub_generate_uuid.c ../core/pubnub_blocking_io.c  ../core/pubnub_timers.c ../core/pubnub_json_parse.c ../lib/md5/md5.c ../lib/base64/pbbase64.c ../core/pubnub_helper.c  ../posix/pubnub_version_posix.c ../posix/pubnub_generate_uuid_posix.c ../posix/pbpal_posix_blocking_io.c ../core/pubnub_free_with_timeout_std.c pubnub_subloop.cpp

ifndef USE_PROXY
USE_PROXY = 1
endif

ifeq ($(USE_PROXY), 1)
SOURCEFILES += ../core/pubnub_proxy.c ../core/pubnub_proxy_core.c ../core/pbhttp_digest.c ../core/pbntlm_core.c ../core/pbntlm_packer_std.c
endif

OS := $(shell uname)
ifeq ($(OS),Darwin)
LDLIBS=-lpthread
else
LDLIBS=-lrt -lpthread
endif

CFLAGS =-g -I ../core -I ../posix -I . -I ../lib/base64 -I ../lib/md5 -Wall -D PUBNUB_THREADSAFE
# -g enables debugging, remove to get a smaller executable


all: cpp11 cpp98

cpp98: pubnub_sync_sample pubnub_callback_sample cancel_subscribe_sync_sample subscribe_publish_callback_sample futres_nesting_sync futres_nesting_callback pubnub_sync_subloop_sample pubnub_callback_subloop_sample

cpp11: pubnub_callback_cpp11_sample futres_nesting_callback_cpp11 fntest_runner pubnub_callback_cpp11_subloop_sample


pubnub_sync_sample: samples/pubnub_sample.cpp $(SOURCEFILES) ../core/pubnub_ntf_sync.c pubnub_futres_sync.cpp
	$(CXX) -o $@ $(CFLAGS) samples/pubnub_sample.cpp ../core/pubnub_ntf_sync.c pubnub_futres_sync.cpp $(SOURCEFILES) $(LDLIBS)

cancel_subscribe_sync_sample: samples/cancel_subscribe_sync_sample.cpp $(SOURCEFILES) ../core/pubnub_ntf_sync.c pubnub_futres_sync.cpp
	$(CXX) -o $@ $(CFLAGS) samples/cancel_subscribe_sync_sample.cpp ../core/pubnub_ntf_sync.c pubnub_futres_sync.cpp $(SOURCEFILES) $(LDLIBS)

futres_nesting_sync: samples/futres_nesting.cpp $(SOURCEFILES) ../core/pubnub_ntf_sync.c pubnub_futres_sync.cpp
	$(CXX) -o $@ $(CFLAGS) samples/futres_nesting.cpp ../core/pubnub_ntf_sync.c pubnub_futres_sync.cpp $(SOURCEFILES) $(LDLIBS)

pubnub_sync_subloop_sample: samples/pubnub_subloop_sample.cpp $(SOURCEFILES) ../core/pubnub_ntf_sync.c pubnub_futres_sync.cpp
	$(CXX) -o $@ $(CFLAGS) samples/pubnub_subloop_sample.cpp ../core/pubnub_ntf_sync.c pubnub_futres_sync.cpp $(SOURCEFILES) $(LDLIBS)

pubnub_callback_sample: samples/pubnub_sample.cpp $(SOURCEFILES) ../core/pubnub_timer_list.c ../posix/pubnub_ntf_callback_posix.c ../posix/pubnub_get_native_socket.c pubnub_futres_posix.cpp
	$(CXX) -o $@ -D PUBNUB_CALLBACK_API $(CFLAGS) -D PUBNUB_LOG_LEVEL=PUBNUB_LOG_LEVEL_TRACE samples/pubnub_sample.cpp ../core/pubnub_timer_list.c ../posix/pubnub_ntf_callback_posix.c ../posix/pubnub_get_native_socket.c pubnub_futres_posix.cpp $(SOURCEFILES) $(LDLIBS) $(LDLIBS)

pubnub_callback_cpp11_sample: samples/pubnub_sample.cpp $(SOURCEFILES) ../core/pubnub_timer_list.c ../posix/pubnub_ntf_callback_posix.c ../posix/pubnub_get_native_socket.c pubnub_futres_cpp11.cpp
	$(CXX) -o $@ -std=c++11 -D PUBNUB_CALLBACK_API $(CFLAGS) -D PUBNUB_LOG_LEVEL=PUBNUB_LOG_LEVEL_TRACE samples/pubnub_sample.cpp ../core/pubnub_timer_list.c ../posix/pubnub_ntf_callback_posix.c ../posix/pubnub_get_native_socket.c pubnub_futres_cpp11.cpp $(SOURCEFILES) $(LDLIBS)

subscribe_publish_callback_sample: samples/subscribe_publish_callback_sample.cpp $(SOURCEFILES) ../core/pubnub_timer_list.c ../posix/pubnub_ntf_callback_posix.c ../posix/pubnub_get_native_socket.c pubnub_futres_posix.cpp
	$(CXX) -o $@ -D PUBNUB_CALLBACK_API $(CFLAGS) samples/subscribe_publish_callback_sample.cpp ../core/pubnub_timer_list.c ../posix/pubnub_ntf_callback_posix.c ../posix/pubnub_get_native_socket.c pubnub_futres_posix.cpp $(SOURCEFILES) $(LDLIBS)

futres_nesting_callback: samples/futres_nesting.cpp $(SOURCEFILES) ../core/pubnub_timer_list.c ../posix/pubnub_ntf_callback_posix.c ../posix/pubnub_get_native_socket.c pubnub_futres_posix.cpp
	$(CXX) -o $@ -D PUBNUB_CALLBACK_API $(CFLAGS)  samples/futres_nesting.cpp ../core/pubnub_timer_list.c ../posix/pubnub_ntf_callback_posix.c ../posix/pubnub_get_native_socket.c pubnub_futres_posix.cpp $(SOURCEFILES) $(LDLIBS)

futres_nesting_callback_cpp11: samples/futres_nesting.cpp $(SOURCEFILES) ../core/pubnub_timer_list.c ../posix/pubnub_ntf_callback_posix.c ../posix/pubnub_get_native_socket.c pubnub_futres_cpp11.cpp
	$(CXX) -o $@ -std=c++11 -D PUBNUB_CALLBACK_API $(CFLAGS)  samples/futres_nesting.cpp ../core/pubnub_timer_list.c ../posix/pubnub_ntf_callback_posix.c ../posix/pubnub_get_native_socket.c pubnub_futres_cpp11.cpp $(SOURCEFILES) $(LDLIBS)

pubnub_callback_subloop_sample: samples/pubnub_subloop_sample.cpp $(SOURCEFILES) ../core/pubnub_timer_list.c ../posix/pubnub_ntf_callback_posix.c ../posix/pubnub_get_native_socket.c pubnub_futres_posix.cpp
	$(CXX) -o $@ -D PUBNUB_CALLBACK_API $(CFLAGS)  samples/pubnub_subloop_sample.cpp ../core/pubnub_timer_list.c ../posix/pubnub_ntf_callback_posix.c ../posix/pubnub_get_native_socket.c pubnub_futres_posix.cpp $(SOURCEFILES) $(LDLIBS)

pubnub_callback_cpp11_subloop_sample: samples/pubnub_subloop_sample.cpp $(SOURCEFILES) ../core/pubnub_timer_list.c ../posix/pubnub_ntf_callback_posix.c ../posix/pubnub_get_native_socket.c pubnub_futres_cpp11.cpp
	$(CXX) -o $@ -std=c++11 -D PUBNUB_CALLBACK_API $(CFLAGS)  samples/pubnub_subloop_sample.cpp ../core/pubnub_timer_list.c ../posix/pubnub_ntf_callback_posix.c ../posix/pubnub_get_native_socket.c pubnub_futres_cpp11.cpp $(SOURCEFILES) $(LDLIBS)

fntest_runner: fntest/pubnub_fntest_runner.cpp $(SOURCEFILES)  ../core/pubnub_ntf_sync.c pubnub_futres_sync.cpp fntest/pubnub_fntest.cpp fntest/pubnub_fntest_basic.cpp fntest/pubnub_fntest_medium.cpp
	$(CXX) -o $@ -std=c++11 -D PUBNUB_LOG_LEVEL=PUBNUB_LOG_LEVEL_TRACE $(CFLAGS) fntest/pubnub_fntest_runner.cpp ../core/pubnub_ntf_sync.c pubnub_futres_sync.cpp fntest/pubnub_fntest.cpp fntest/pubnub_fntest_basic.cpp fntest/pubnub_fntest_medium.cpp $(SOURCEFILES) $(LDLIBS) 


clean:
	rm pubnub_sync_sample pubnub_callback_sample pubnub_callback_cpp11_sample cancel_subscribe_sync_sample subscribe_publish_callback_sample futres_nesting_sync futres_nesting_callback futres_nesting_callback_cpp11 fntest_runner
