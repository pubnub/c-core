SOURCEFILES += ../core/pubnub_coreapi.c ../core/pubnub_ccore.c ../core/pubnub_netcore.c  pbpal_openssl.c pbpal_resolv_and_connect_openssl.c ../core/pubnub_alloc_std.c ../core/pubnub_assert_std.c ../core/pubnub_generate_uuid.c pubnub_version_openssl.c  ../posix/pubnub_generate_uuid_posix.c

CFLAGS = -Wall -D VERBOSE_DEBUG -I ../core -I . -I ../posix

all: pubnub_sync_sample pubnub_callback_sample


pubnub_sync_sample: ../core/samples/pubnub_sync_sample.c $(SOURCEFILES) ../core/pubnub_ntf_sync.c
	gcc -o pubnub_sync_sample $(CFLAGS) ../core/samples/pubnub_sync_sample.c ../core/pubnub_ntf_sync.c $(SOURCEFILES) -lssl -lcrypto

pubnub_callback_sample: ../core/samples/pubnub_callback_sample.c $(SOURCEFILES) ../posix/pubnub_ntf_callback_posix.c
	gcc -o pubnub_callback_sample $(CFLAGS) ../core/samples/pubnub_callback_sample.c ../posix/pubnub_ntf_callback_posix.c $(SOURCEFILES) -lpthread -lssl -lcrypto

clean:
	rm pubnub_sync_sample pubnub_callback_sample
