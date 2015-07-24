SOURCEFILES = ..\core\pubnub_coreapi.c ..\core\pubnub_ccore.c ..\core\pubnub_netcore.c  ..\lib\sockets\pbpal_sockets.c ..\lib/sockets\pbpal_resolv_and_connect_sockets.c ..\core\pubnub_alloc_std.c ..\core\pubnub_assert_std.c ..\core\pubnub_generate_uuid.c ..\core\pubnub_blocking_io.c pubnub_version_windows.c  pubnub_generate_uuid_windows.c pbpal_windows_blocking_io.c ..\core\c99\snprintf.c

CFLAGS = -D VERBOSE_DEBUG -I ..\core -I ..\core\c99 -I . /W3 

all: pubnub_sync_sample.exe pubnub_callback_sample.exe

pubnub_sync_sample.exe: ..\core\samples\pubnub_sync_sample.c $(SOURCEFILES) ..\core\pubnub_ntf_sync.c
	cl $(CFLAGS) ..\core\samples\pubnub_sync_sample.c $(SOURCEFILES) ..\core\pubnub_ntf_sync.c ws2_32.lib rpcrt4.lib

pubnub_callback_sample.exe: ..\core\samples\pubnub_callback_sample.c $(SOURCEFILES) pubnub_ntf_callback_windows.c
	cl $(CFLAGS) ..\core\samples\pubnub_callback_sample.c  $(SOURCEFILES) pubnub_ntf_callback_windows.c ws2_32.lib rpcrt4.lib

clean:
	del pubnub_posix_sample.exe pubnub_callback_sample.exe

