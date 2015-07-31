SOURCEFILES = ../core/pubnub_coreapi.c ../core/pubnub_ccore.c ../core/pubnub_netcore.c  ../lib/sockets/pbpal_sockets.c ../lib/sockets/pbpal_resolv_and_connect_sockets.c ../core/pubnub_alloc_std.c ../core/pubnub_assert_std.c ../core/pubnub_generate_uuid.c ../core/pubnub_blocking_io.c ../core/pubnub_helper.c  pubnub_version_posix.c  pubnub_generate_uuid_posix.c pbpal_posix_blocking_io.c

CFLAGS = -D VERBOSE_DEBUG -I ../core -I . -I fntest -I ../core/fntest -Wall 

all: pubnub_sync_sample pubnub_callback_sample pubnub_fntest


pubnub_sync_sample: ../core/samples/pubnub_sync_sample.c $(SOURCEFILES) ../core/pubnub_ntf_sync.c
	gcc -o pubnub_sync_sample $(CFLAGS) ../core/samples/pubnub_sync_sample.c ../core/pubnub_ntf_sync.c $(SOURCEFILES)

pubnub_callback_sample: ../core/samples/pubnub_callback_sample.c $(SOURCEFILES) pubnub_ntf_callback_posix.c
	gcc -o pubnub_callback_sample $(CFLAGS) ../core/samples/pubnub_callback_sample.c pubnub_ntf_callback_posix.c $(SOURCEFILES) -lpthread

pubnub_fntest: ../core/fntest/pubnub_fntest.c ../core/fntest/pubnub_fntest_basic.c fntest/pubnub_fntest_posix.c fntest/pubnub_fntest_runner.c $(SOURCEFILES)  ../core/pubnub_ntf_sync.c
	gcc -o pubnub_fntest $(CFLAGS) ../core/fntest/pubnub_fntest.c ../core/fntest/pubnub_fntest_basic.c fntest/pubnub_fntest_posix.c fntest/pubnub_fntest_runner.c $(SOURCEFILES)  ../core/pubnub_ntf_sync.c -lpthread

clean:
	rm pubnub_sync_sample pubnub_callback_sample pubnub_fntest
