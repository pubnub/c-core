SOURCEFILES = ../core/pubnub_coreapi.c ../core/pubnub_ccore.c ../core/pubnub_netcore.c  ../openssl/pbpal_openssl.c ../openssl/pbpal_resolv_and_connect_openssl.c  ../core/pubnub_alloc_std.c ../core/pubnub_assert_std.c ../core/pubnub_generate_uuid.c ../core/pubnub_blocking_io.c ../core/pubnub_timers.c ../core/pubnub_json_parse.c ../core/pubnub_helper.c  ../openssl/pubnub_version_openssl.c ../posix/pubnub_generate_uuid_posix.c ../openssl/pbpal_openssl_blocking_io.c

OS := $(shell uname)
ifeq ($(OS),Darwin)
SOURCEFILES += ../posix/monotonic_clock_get_time_darwin.c
LDLIBS=-lpthread -lssl -lcrypto
else
SOURCEFILES += ../posix/monotonic_clock_get_time_posix.c
LDLIBS=-lrt -lpthread -lssl -lcrypto
endif

CFLAGS =-g -I ../core -I ../openssl -I . -Wall -D PUBNUB_THREADSAFE
# -g enables debugging, remove to get a smaller executable

all: openssl/pubnub_sync_sample openssl/pubnub_callback_sample openssl/pubnub_callback_cpp11_sample openssl/cancel_subscribe_sync_sample openssl/subscribe_publish_callback_sample openssl/futres_nesting_sync openssl/futres_nesting_callback openssl/futres_nesting_callback_cpp11


openssl/pubnub_sync_sample: samples/pubnub_sample.cpp $(SOURCEFILES) ../core/pubnub_ntf_sync.c pubnub_futres_sync.cpp
	$(CXX) -o $@ $(CFLAGS) samples/pubnub_sample.cpp ../core/pubnub_ntf_sync.c pubnub_futres_sync.cpp $(SOURCEFILES) $(LDLIBS)

openssl/cancel_subscribe_sync_sample: samples/cancel_subscribe_sync_sample.cpp $(SOURCEFILES) ../core/pubnub_ntf_sync.c pubnub_futres_sync.cpp
	$(CXX) -o $@ $(CFLAGS) samples/cancel_subscribe_sync_sample.cpp  ../core/pubnub_ntf_sync.c pubnub_futres_sync.cpp $(SOURCEFILES) $(LDLIBS)

openssl/futres_nesting_sync: samples/futres_nesting.cpp $(SOURCEFILES) ../core/pubnub_ntf_sync.c pubnub_futres_sync.cpp
	$(CXX) -o $@ $(CFLAGS) samples/futres_nesting.cpp ../core/pubnub_ntf_sync.c pubnub_futres_sync.cpp $(SOURCEFILES) $(LDLIBS)

openssl/pubnub_callback_sample: samples/pubnub_sample.cpp $(SOURCEFILES) ../core/pubnub_timer_list.c ../openssl/pubnub_ntf_callback_posix.c ../openssl/pubnub_get_native_socket.c pubnub_futres_posix.cpp
	$(CXX) -o $@ -D PUBNUB_CALLBACK_API $(CFLAGS) samples/pubnub_sample.cpp ../core/pubnub_timer_list.c ../openssl/pubnub_ntf_callback_posix.c ../openssl/pubnub_get_native_socket.c pubnub_futres_posix.cpp $(SOURCEFILES) $(LDLIBS)

openssl/pubnub_callback_cpp11_sample: samples/pubnub_sample.cpp $(SOURCEFILES) ../core/pubnub_timer_list.c ../openssl/pubnub_ntf_callback_posix.c ../openssl/pubnub_get_native_socket.c pubnub_futres_cpp11.cpp
	$(CXX) -o $@ --std=c++11 -D PUBNUB_CALLBACK_API $(CFLAGS) samples/pubnub_sample.cpp ../core/pubnub_timer_list.c ../openssl/pubnub_ntf_callback_posix.c ../openssl/pubnub_get_native_socket.c pubnub_futres_cpp11.cpp $(SOURCEFILES) $(LDLIBS)

openssl/subscribe_publish_callback_sample: samples/subscribe_publish_callback_sample.cpp $(SOURCEFILES) ../core/pubnub_timer_list.c ../openssl/pubnub_ntf_callback_posix.c ../openssl/pubnub_get_native_socket.c pubnub_futres_posix.cpp
	$(CXX) -o $@ -D PUBNUB_CALLBACK_API $(CFLAGS) samples/subscribe_publish_callback_sample.cpp ../core/pubnub_timer_list.c ../openssl/pubnub_ntf_callback_posix.c ../openssl/pubnub_get_native_socket.c pubnub_futres_posix.cpp $(SOURCEFILES) $(LDLIBS)

openssl/futres_nesting_callback: samples/futres_nesting.cpp $(SOURCEFILES) ../core/pubnub_timer_list.c ../openssl/pubnub_ntf_callback_posix.c ../openssl/pubnub_get_native_socket.c pubnub_futres_posix.cpp
	$(CXX) -o $@ -D PUBNUB_CALLBACK_API $(CFLAGS)  samples/futres_nesting.cpp ../core/pubnub_timer_list.c ../openssl/pubnub_ntf_callback_posix.c ../openssl/pubnub_get_native_socket.c pubnub_futres_posix.cpp $(SOURCEFILES) $(LDLIBS)

openssl/futres_nesting_callback_cpp11: samples/futres_nesting.cpp $(SOURCEFILES) ../core/pubnub_timer_list.c ../openssl/pubnub_ntf_callback_posix.c ../openssl/pubnub_get_native_socket.c pubnub_futres_cpp11.cpp
	$(CXX) -o $@ --std=c++11 -D PUBNUB_CALLBACK_API $(CFLAGS)  samples/futres_nesting.cpp ../core/pubnub_timer_list.c ../openssl/pubnub_ntf_callback_posix.c ../openssl/pubnub_get_native_socket.c pubnub_futres_cpp11.cpp $(SOURCEFILES) $(LDLIBS)


clean:
	rm openssl/pubnub_sync_sample openssl/pubnub_callback_sample openssl/pubnub_callback_cpp11_sample openssl/cancel_subscribe_sync_sample openssl/subscribe_publish_callback_sample openssl/futres_nesting_sync openssl/futres_nesting_callback openssl/futres_nesting_callback_cpp11
