SOURCEFILES = ../core/pubnub_coreapi.c ../core/pubnub_coreapi_ex.c ../core/pubnub_ccore.c ../core/pubnub_netcore.c  ../lib/sockets/pbpal_sockets.c ../lib/sockets/pbpal_resolv_and_connect_sockets.c ../core/pubnub_alloc_std.c ../core/pubnub_assert_std.c ../core/pubnub_generate_uuid.c ../core/pubnub_blocking_io.c ../core/pubnub_timers.c ../core/pubnub_json_parse.c ../core/pubnub_proxy.c ../core/pubnub_helper.c  pubnub_version_posix.c  pubnub_generate_uuid_posix.c pbpal_posix_blocking_io.c

OBJFILES = pubnub_coreapi.o pubnub_coreapi_ex.o pubnub_ccore.o pubnub_netcore.o  pbpal_sockets.o pbpal_resolv_and_connect_sockets.o pubnub_alloc_std.o pubnub_assert_std.o pubnub_generate_uuid.o pubnub_blocking_io.o pubnub_timers.o pubnub_json_parse.o pubnub_proxy.o pubnub_helper.o  pubnub_version_posix.o  pubnub_generate_uuid_posix.o pbpal_posix_blocking_io.o

OS := $(shell uname)
ifeq ($(OS),Darwin)
SOURCEFILES += monotonic_clock_get_time_darwin.c
OBJFILES += monotonic_clock_get_time_darwin.o
LDLIBS=-lpthread
else
SOURCEFILES += monotonic_clock_get_time_posix.c
OBJFILES += monotonic_clock_get_time_posix.o
LDLIBS=-lrt -lpthread
endif

CFLAGS =-g -I ../core -I . -I fntest -I ../core/fntest -Wall -D PUBNUB_THREADSAFE -D PUBNUB_LOG_LEVEL=PUBNUB_LOG_LEVEL_NONE
# -g enables debugging, remove to get a smaller executable
# -fsanitize=address Use AddressSanitizer


all: pubnub_sync_sample cancel_subscribe_sync_sample pubnub_callback_sample subscribe_publish_callback_sample pubnub_fntest pubnub_console_sync pubnub_console_callback

pubnub_sync.a : $(SOURCEFILES) ../core/pubnub_ntf_sync.c
	gcc -c $(CFLAGS) $(SOURCEFILES) ../core/pubnub_ntf_sync.c
	ar rcs pubnub_sync.a $(OBJFILES) pubnub_ntf_sync.o

pubnub_callback.a : $(SOURCEFILES)  ../core/pubnub_timer_list.c pubnub_ntf_callback_posix.c pubnub_get_native_socket.c ../lib/sockets/pbpal_adns_sockets.c
	gcc -c $(CFLAGS) -D PUBNUB_CALLBACK_API $(SOURCEFILES)  ../core/pubnub_timer_list.c pubnub_ntf_callback_posix.c pubnub_get_native_socket.c ../lib/sockets/pbpal_adns_sockets.c
	ar rcs pubnub_callback.a $(OBJFILES)  pubnub_timer_list.o pubnub_ntf_callback_posix.o pubnub_get_native_socket.o pbpal_adns_sockets.o

pubnub_sync_sample: ../core/samples/pubnub_sync_sample.c pubnub_sync.a
	$(CC) -o $@ $(CFLAGS) ../core/samples/pubnub_sync_sample.c pubnub_sync.a $(LDLIBS)

cancel_subscribe_sync_sample: ../core/samples/cancel_subscribe_sync_sample.c pubnub_sync.a
	$(CC) -o $@ $(CFLAGS) ../core/samples/cancel_subscribe_sync_sample.c pubnub_sync.a $(LDLIBS)

pubnub_callback_sample: ../core/samples/pubnub_callback_sample.c pubnub_callback.a
	$(CC) -o $@ -D PUBNUB_CALLBACK_API $(CFLAGS) ../core/samples/pubnub_callback_sample.c pubnub_callback.a $(LDLIBS)

subscribe_publish_callback_sample: ../core/samples/subscribe_publish_callback_sample.c pubnub_callback.a
	$(CC) -o $@ -D PUBNUB_CALLBACK_API $(CFLAGS) ../core/samples/subscribe_publish_callback_sample.c pubnub_callback.a $(LDLIBS)

pubnub_fntest: ../core/fntest/pubnub_fntest.c ../core/fntest/pubnub_fntest_basic.c ../core/fntest/pubnub_fntest_medium.c fntest/pubnub_fntest_posix.c fntest/pubnub_fntest_runner.c pubnub_sync.a
	$(CC) -o $@ $(CFLAGS) ../core/fntest/pubnub_fntest.c ../core/fntest/pubnub_fntest_basic.c ../core/fntest/pubnub_fntest_medium.c  fntest/pubnub_fntest_posix.c fntest/pubnub_fntest_runner.c pubnub_sync.a $(LDLIBS) -lpthread

CONSOLE_SOURCEFILES=../core/samples/console/pubnub_console.c ../core/samples/console/pnc_helpers.c ../core/samples/console/pnc_readers.c ../core/samples/console/pnc_subscriptions.c

pubnub_console_sync: $(CONSOLE_SOURCEFILES) ../core/samples/console/pnc_ops_sync.c pubnub_sync.a
	$(CC) -o $@ $(CFLAGS) $(CONSOLE_SOURCEFILES) ../core/samples/console/pnc_ops_sync.c  pubnub_sync.a $(LDLIBS)

pubnub_console_callback: $(CONSOLE_SOURCEFILES) ../core/samples/console/pnc_ops_callback.c pubnub_callback.a
	$(CC) -o $@ $(CFLAGS) -D PUBNUB_CALLBACK_API $(CONSOLE_SOURCEFILES) ../core/samples/console/pnc_ops_callback.c pubnub_callback.a $(LDLIBS)


clean:
	rm pubnub_sync_sample cancel_subscribe_sync_sample pubnub_callback_sample subscribe_publish_callback_sample pubnub_fntest pubnub_console_sync pubnub_console_callback pubnub_sync.a pubnub_callback.a
