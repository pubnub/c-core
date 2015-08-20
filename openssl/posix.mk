SOURCEFILES += ../core/pubnub_coreapi.c ../core/pubnub_ccore.c ../core/pubnub_netcore.c  pbpal_openssl.c pbpal_resolv_and_connect_openssl.c ../core/pubnub_alloc_std.c ../core/pubnub_assert_std.c ../core/pubnub_generate_uuid.c ../core/pubnub_blocking_io.c ../core/pubnub_json_parse.c ../core/pubnub_helper.c pubnub_version_openssl.c  ../posix/pubnub_generate_uuid_posix.c pbpal_openssl_blocking_io.c

CFLAGS =-g -D VERBOSE_DEBUG -I ../core -I . -I ../posix/fntest -I ../core/fntest -I ../posix -Wall
# -g enables debugging, remove to get a smaller executable

all: pubnub_sync_sample pubnub_callback_sample subscribe_publish_callback_sample pubnub_fntest


pubnub_sync_sample: ../core/samples/pubnub_sync_sample.c $(SOURCEFILES) ../core/pubnub_ntf_sync.c
	$(CC) -o pubnub_sync_sample $(CFLAGS) ../core/samples/pubnub_sync_sample.c ../core/pubnub_ntf_sync.c $(SOURCEFILES) -lssl -lcrypto

pubnub_callback_sample: ../core/samples/pubnub_callback_sample.c $(SOURCEFILES) ../posix/pubnub_ntf_callback_posix.c
	$(CC) -o pubnub_callback_sample $(CFLAGS) -D PUBNUB_CALLBACK_API ../core/samples/pubnub_callback_sample.c ../posix/pubnub_ntf_callback_posix.c $(SOURCEFILES) -lpthread -lssl -lcrypto

subscribe_publish_callback_sample: ../core/samples/subscribe_publish_callback_sample.c $(SOURCEFILES) ../posix/pubnub_ntf_callback_posix.c
	$(CC) -o subscribe_publish_callback_sample -D PUBNUB_CALLBACK_API $(CFLAGS) ../core/samples/subscribe_publish_callback_sample.c ../posix/pubnub_ntf_callback_posix.c $(SOURCEFILES) -lpthread -lssl -lcrypto

pubnub_fntest: ../core/fntest/pubnub_fntest.c ../core/fntest/pubnub_fntest_basic.c ../core/fntest/pubnub_fntest_medium.c ../posix/fntest/pubnub_fntest_posix.c ../posix/fntest/pubnub_fntest_runner.c $(SOURCEFILES)  ../core/pubnub_ntf_sync.c
	$(CC) -o pubnub_fntest $(CFLAGS) ../core/fntest/pubnub_fntest.c ../core/fntest/pubnub_fntest_basic.c ../core/fntest/pubnub_fntest_medium.c  ../posix/fntest/pubnub_fntest_posix.c ../posix/fntest/pubnub_fntest_runner.c $(SOURCEFILES)  ../core/pubnub_ntf_sync.c -lpthread -lssl -lcrypto

clean:
	rm pubnub_sync_sample pubnub_callback_sample subscribe_publish_callback_sample pubnub_fntest
