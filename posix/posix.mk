SOURCEFILES = ../core/pubnub_coreapi.c ../core/pubnub_coreapi_ex.c ../core/pubnub_ccore.c ../core/pubnub_netcore.c  ../lib/sockets/pbpal_sockets.c ../lib/sockets/pbpal_resolv_and_connect_sockets.c ../core/pubnub_alloc_std.c ../core/pubnub_assert_std.c ../core/pubnub_generate_uuid.c ../core/pubnub_blocking_io.c ../core/pubnub_timers.c ../core/pubnub_json_parse.c ../core/pubnub_helper.c  pubnub_version_posix.c  pubnub_generate_uuid_posix.c pbpal_posix_blocking_io.c
HEADERS = $(wildcard ../core/*.h) $(wildcard ./*.h)

OS := $(shell uname)
ifeq ($(OS),Darwin)
SOURCEFILES += monotonic_clock_get_time_darwin.c
LDLIBS=-lpthread
else
SOURCEFILES += monotonic_clock_get_time_posix.c
LDLIBS=-lrt -lpthread
endif

CFLAGS =-g -I ../core -I . -I fntest -I ../core/fntest -Wall -D PUBNUB_THREADSAFE -D PUBNUB_LOG_LEVEL=PUBNUB_LOG_LEVEL_NONE
# -g enables debugging, remove to get a smaller executable
# -fsanitize=address Use AddressSanitizer


all: pubnub_sync_sample cancel_subscribe_sync_sample pubnub_callback_sample subscribe_publish_callback_sample pubnub_fntest pubnub_console_sync pubnub_console_callback


pubnub_sync_sample: ../core/samples/pubnub_sync_sample.c $(SOURCEFILES) ../core/pubnub_ntf_sync.c $(HEADERS)
	$(CC) -o $@ $(CFLAGS) ../core/samples/pubnub_sync_sample.c ../core/pubnub_ntf_sync.c $(SOURCEFILES) $(LDLIBS)

cancel_subscribe_sync_sample: ../core/samples/cancel_subscribe_sync_sample.c $(SOURCEFILES) ../core/pubnub_ntf_sync.c $(HEADERS)
	$(CC) -o $@ $(CFLAGS) ../core/samples/cancel_subscribe_sync_sample.c ../core/pubnub_ntf_sync.c $(SOURCEFILES) $(LDLIBS)

pubnub_callback_sample: ../core/samples/pubnub_callback_sample.c $(SOURCEFILES) ../core/pubnub_timer_list.c pubnub_ntf_callback_posix.c pubnub_get_native_socket.c ../lib/sockets/pbpal_adns_sockets.c $(HEADERS)
	$(CC) -o $@ -D PUBNUB_CALLBACK_API $(CFLAGS) -D PUBNUB_USE_ADNS=1 ../core/samples/pubnub_callback_sample.c ../core/pubnub_timer_list.c pubnub_ntf_callback_posix.c pubnub_get_native_socket.c ../lib/sockets/pbpal_adns_sockets.c $(SOURCEFILES) $(LDLIBS)

subscribe_publish_callback_sample: ../core/samples/subscribe_publish_callback_sample.c $(SOURCEFILES) ../core/pubnub_timer_list.c pubnub_ntf_callback_posix.c pubnub_get_native_socket.c $(HEADERS)
	$(CC) -o $@ -D PUBNUB_CALLBACK_API $(CFLAGS) ../core/samples/subscribe_publish_callback_sample.c ../core/pubnub_timer_list.c pubnub_ntf_callback_posix.c pubnub_get_native_socket.c $(SOURCEFILES) $(LDLIBS)

pubnub_fntest: ../core/fntest/pubnub_fntest.c ../core/fntest/pubnub_fntest_basic.c ../core/fntest/pubnub_fntest_medium.c fntest/pubnub_fntest_posix.c fntest/pubnub_fntest_runner.c $(SOURCEFILES)  ../core/pubnub_ntf_sync.c $(HEADERS)
	$(CC) -o $@ $(CFLAGS) -U PUBNUB_LOG_LEVEL -D PUBNUB_LOG_LEVEL=PUBNUB_LOG_LEVEL_TRACE ../core/fntest/pubnub_fntest.c ../core/fntest/pubnub_fntest_basic.c ../core/fntest/pubnub_fntest_medium.c  fntest/pubnub_fntest_posix.c fntest/pubnub_fntest_runner.c $(SOURCEFILES)  ../core/pubnub_ntf_sync.c $(LDLIBS) -lpthread

CONSOLE_SOURCEFILES=../core/samples/console/pubnub_console.c ../core/samples/console/pnc_helpers.c ../core/samples/console/pnc_readers.c ../core/samples/console/pnc_subscriptions.c

pubnub_console_sync: $(CONSOLE_SOURCEFILES) ../core/samples/console/pnc_ops_sync.c  $(SOURCEFILES) ../core/pubnub_ntf_sync.c  $(HEADERS)
	$(CC) -o $@ $(CFLAGS) $(CONSOLE_SOURCEFILES) ../core/samples/console/pnc_ops_sync.c  $(SOURCEFILES) ../core/pubnub_ntf_sync.c $(LDLIBS)

pubnub_console_callback: $(CONSOLE_SOURCEFILES) ../core/samples/console/pnc_ops_callback.c $(SOURCEFILES) ../core/pubnub_timer_list.c pubnub_ntf_callback_posix.c pubnub_get_native_socket.c $(HEADERS)
	$(CC) -o $@ $(CFLAGS) -D PUBNUB_CALLBACK_API $(CONSOLE_SOURCEFILES) ../core/samples/console/pnc_ops_callback.c $(SOURCEFILES) ../core/pubnub_timer_list.c pubnub_ntf_callback_posix.c pubnub_get_native_socket.c $(LDLIBS)


clean:
	rm pubnub_sync_sample cancel_subscribe_sync_sample pubnub_callback_sample subscribe_publish_callback_sample pubnub_fntest pubnub_console_sync pubnub_console_callback
