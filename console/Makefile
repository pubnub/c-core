CC = gcc
PROJECT_SOURCEFILES +=  pubnub_console.c pnc_helpers.c pnc_readers.c ../core/pubnub_alloc_static.c ../core/pubnub_assert_std.c ../core/pubnub_ccore.c ../core/pubnub_coreapi.c ../posix/pubnub_version_posix.c ../lib/sockets/pbpal_sockets.c ../core/pubnub_netcore.c   ../lib/sockets/pbpal_resolv_and_connect_sockets.c ../core/pubnub_blocking_io.c ../posix/pbpal_posix_blocking_io.c
SYNC_SOURCEFILES += ../core/pubnub_ntf_sync.c pnc_ops_sync.c
CALLBACK_POSIX_SOURCEFILES += ../posix/pubnub_ntf_callback_posix.c pnc_ops_callback.c

CFLAGS += -Wall -I../posix -I../core -I.
#CFLAGS += -D VERBOSE_DEBUG

all: clean pubnub_posix_sync_console pubnub_posix_callback_console
#all: clean pubnub_posix_callback_console
#all: clean pubnub_posix_sync_console

pubnub_posix_sync_console: pubnub_console.c pnc_ops_sync.c pnc_helpers.c pnc_readers.c
	$(CC) -o pubnub_posix_sync_console $(CFLAGS) \
		$(SYNC_SOURCEFILES) $(PROJECT_SOURCEFILES)

pubnub_posix_callback_console: pubnub_console.c pnc_ops_callback.c pnc_helpers.c pnc_readers.c
	$(CC) -o pubnub_posix_callback_console $(CFLAGS) \
		$(CALLBACK_POSIX_SOURCEFILES) $(PROJECT_SOURCEFILES) -lpthread

clean:
	rm -rf *.o pubnub_posix_sync_console pubnub_posix_callback_console

