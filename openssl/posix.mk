SOURCEFILES = ../core/pubnub_coreapi.c ../core/pubnub_ccore.c ../core/pubnub_netcore.c  pbpal_openssl.c pbpal_resolv_and_connect_openssl.c ../core/pubnub_alloc_std.c ../core/pubnub_assert_std.c ../core/pubnub_generate_uuid.c ../core/pubnub_blocking_io.c ../core/pubnub_timers.c ../core/pubnub_json_parse.c ../core/pubnub_helper.c pubnub_version_openssl.c ../posix/pubnub_generate_uuid_posix.c pbpal_openssl_blocking_io.c

OS := $(shell uname)
ifeq ($(OS),Darwin)
SOURCEFILES += ../posix/monotonic_clock_get_time_darwin.c
else
SOURCEFILES += ../posix/monotonic_clock_get_time_posix.c
endif

CFLAGS =-g -D VERBOSE_DEBUG -I ../core -I . -I ../posix/fntest -I ../core/fntest -Wall -D PUBNUB_THREADSAFE
# -g enables debugging, remove to get a smaller executable

all: pubnub_sync_sample cancel_subscribe_sync_sample pubnub_callback_sample subscribe_publish_callback_sample pubnub_fntest


pubnub_sync_sample: ../core/samples/pubnub_sync_sample.c $(SOURCEFILES) ../core/pubnub_ntf_sync.c
	$(CC) -o $@ $(CFLAGS) ../core/samples/pubnub_sync_sample.c ../core/pubnub_ntf_sync.c $(SOURCEFILES) -lssl -lcrypto

cancel_subscribe_sync_sample: ../core/samples/cancel_subscribe_sync_sample.c $(SOURCEFILES) ../core/pubnub_ntf_sync.c
	$(CC) -o $@ $(CFLAGS) ../core/samples/cancel_subscribe_sync_sample.c ../core/pubnub_ntf_sync.c $(SOURCEFILES) -lssl -lcrypto

pubnub_callback_sample: ../core/samples/pubnub_callback_sample.c $(SOURCEFILES)  ../core/pubnub_timer_list.c pubnub_ntf_callback_posix.c pubnub_get_native_socket.c
	$(CC) -o $@ $(CFLAGS) -D PUBNUB_CALLBACK_API ../core/samples/pubnub_callback_sample.c  ../core/pubnub_timer_list.c pubnub_ntf_callback_posix.c pubnub_get_native_socket.c $(SOURCEFILES) -lpthread -lssl -lcrypto

subscribe_publish_callback_sample: ../core/samples/subscribe_publish_callback_sample.c $(SOURCEFILES)  ../core/pubnub_timer_list.c pubnub_ntf_callback_posix.c pubnub_get_native_socket.c
	$(CC) -o $@ -D PUBNUB_CALLBACK_API $(CFLAGS) ../core/samples/subscribe_publish_callback_sample.c  ../core/pubnub_timer_list.c pubnub_ntf_callback_posix.c pubnub_get_native_socket.c $(SOURCEFILES) -lpthread -lssl -lcrypto

pubnub_fntest: ../core/fntest/pubnub_fntest.c ../core/fntest/pubnub_fntest_basic.c ../core/fntest/pubnub_fntest_medium.c ../posix/fntest/pubnub_fntest_posix.c ../posix/fntest/pubnub_fntest_runner.c $(SOURCEFILES)  ../core/pubnub_ntf_sync.c
	$(CC) -o $@ $(CFLAGS) ../core/fntest/pubnub_fntest.c ../core/fntest/pubnub_fntest_basic.c ../core/fntest/pubnub_fntest_medium.c  ../posix/fntest/pubnub_fntest_posix.c ../posix/fntest/pubnub_fntest_runner.c $(SOURCEFILES)  ../core/pubnub_ntf_sync.c -lpthread -lssl -lcrypto

clean:
	rm pubnub_sync_sample cancel_subscribe_sync_sample pubnub_callback_sample subscribe_publish_callback_sample pubnub_fntest
