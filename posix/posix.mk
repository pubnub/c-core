SOURCEFILES = ../core/pubnub_coreapi.c ../core/pubnub_ccore.c ../core/pubnub_netcore.c  ../lib/sockets/pbpal_sockets.c ../lib/sockets/pbpal_resolv_and_connect_sockets.c ../core/pubnub_alloc_std.c ../core/pubnub_assert_std.c ../core/pubnub_generate_uuid.c ../core/pubnub_blocking_io.c ../core/pubnub_json_parse.c ../core/pubnub_helper.c  pubnub_version_posix.c  pubnub_generate_uuid_posix.c pbpal_posix_blocking_io.c

CFLAGS =-g -I ../core -I . -I fntest -I ../core/fntest -Wall
# -g enables debugging, remove to get a smaller executable

all: pubnub_sync_sample cancel_subscribe_sync_sample pubnub_callback_sample subscribe_publish_callback_sample pubnub_fntest


pubnub_sync_sample: ../core/samples/pubnub_sync_sample.c $(SOURCEFILES) ../core/pubnub_ntf_sync.c
	$(CC) -o pubnub_sync_sample $(CFLAGS) -D VERBOSE_DEBUG ../core/samples/pubnub_sync_sample.c ../core/pubnub_ntf_sync.c $(SOURCEFILES)

cancel_subscribe_sync_sample: ../core/samples/cancel_subscribe_sync_sample.c $(SOURCEFILES) ../core/pubnub_ntf_sync.c
	$(CC) -o cancel_subscribe_sync_sample $(CFLAGS) ../core/samples/cancel_subscribe_sync_sample.c ../core/pubnub_ntf_sync.c $(SOURCEFILES)

pubnub_callback_sample: ../core/samples/pubnub_callback_sample.c $(SOURCEFILES) pubnub_ntf_callback_posix.c
	$(CC) -o pubnub_callback_sample -D PUBNUB_CALLBACK_API $(CFLAGS) -D VERBOSE_DEBUG ../core/samples/pubnub_callback_sample.c pubnub_ntf_callback_posix.c $(SOURCEFILES) -lpthread

subscribe_publish_callback_sample: ../core/samples/subscribe_publish_callback_sample.c $(SOURCEFILES) pubnub_ntf_callback_posix.c
	$(CC) -o subscribe_publish_callback_sample -D PUBNUB_CALLBACK_API $(CFLAGS) -D VERBOSE_DEBUG ../core/samples/subscribe_publish_callback_sample.c pubnub_ntf_callback_posix.c $(SOURCEFILES) -lpthread

pubnub_fntest: ../core/fntest/pubnub_fntest.c ../core/fntest/pubnub_fntest_basic.c ../core/fntest/pubnub_fntest_medium.c fntest/pubnub_fntest_posix.c fntest/pubnub_fntest_runner.c $(SOURCEFILES)  ../core/pubnub_ntf_sync.c
	$(CC) -o pubnub_fntest $(CFLAGS) -D VERBOSE_DEBUG ../core/fntest/pubnub_fntest.c ../core/fntest/pubnub_fntest_basic.c ../core/fntest/pubnub_fntest_medium.c  fntest/pubnub_fntest_posix.c fntest/pubnub_fntest_runner.c $(SOURCEFILES)  ../core/pubnub_ntf_sync.c -lpthread

clean:
	rm pubnub_sync_sample pubnub_callback_sample subscribe_publish_callback_sample pubnub_fntest
