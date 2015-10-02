SOURCEFILES = ../core/pubnub_coreapi.c ../core/pubnub_ccore.c ../core/pubnub_netcore.c  ../lib/sockets/pbpal_sockets.c ../lib/sockets/pbpal_resolv_and_connect_sockets.c ../core/pubnub_alloc_std.c ../core/pubnub_assert_std.c ../core/pubnub_generate_uuid.c ../core/pubnub_blocking_io.c ../core/pubnub_json_parse.c ../core/pubnub_helper.c pubnub_version_windows.c  pubnub_generate_uuid_windows.c pbpal_windows_blocking_io.c ../core/c99/snprintf.c

CFLAGS =-g -I ../core -I . -I fntest -I ../core/fntest -Wall
# -g enables debugging, remove to get a smaller executable

LIBFILES=-lws2_32 -lrpcrt4

##
# We don't build the callback examples, because some older versions of MINGW
# don't have WSAPOLLFD defined. If you have a newer version of MING, uncomment
# the callback examples.
#

all: pubnub_sync_sample.exe cancel_subscribe_sync_sample.exe 
#subscribe_publish_callback_sample.exe pubnub_callback_sample.exe pubnub_fntest.exe

pubnub_sync_sample.exe: ../core/samples/pubnub_sync_sample.c $(SOURCEFILES) ../core/pubnub_ntf_sync.c
	$(CC) -o $@ $(CFLAGS) ../core/samples/pubnub_sync_sample.c $(SOURCEFILES) ../core/pubnub_ntf_sync.c $(LIBFILES)

cancel_subscribe_sync_sample.exe: ../core/samples/cancel_subscribe_sync_sample.c $(SOURCEFILES) ../core/pubnub_ntf_sync.c
	$(CC) -o $@ $(CFLAGS) ../core/samples/cancel_subscribe_sync_sample.c $(SOURCEFILES) ../core/pubnub_ntf_sync.c $(LIBFILES)

#pubnub_callback_sample.exe: ../core/samples/pubnub_callback_sample.c $(SOURCEFILES) pubnub_ntf_callback_windows.c
#	$(CC) -o $@ $(CFLAGS) -DPUBNUB_CALLBACK_API ../core/samples/pubnub_callback_sample.c  $(SOURCEFILES) pubnub_ntf_callback_windows.c $(LIBFILES)

#subscribe_publish_callback_sample.exe: ../core/samples/subscribe_publish_callback_sample.c $(SOURCEFILES) pubnub_ntf_callback_windows.c
#	$(CC) -o $@ $(CFLAGS) -DPUBNUB_CALLBACK_API ../core/samples/subscribe_publish_callback_sample.c  $(SOURCEFILES) pubnub_ntf_callback_windows.c $(LIBFILES)

pubnub_fntest.exe: ../core/fntest/pubnub_fntest.c ../core/fntest/pubnub_fntest_basic.c ../core/fntest/pubnub_fntest_medium.c  fntest/pubnub_fntest_windows.c fntest/pubnub_fntest_runner.c $(SOURCEFILES)  ../core/pubnub_ntf_sync.c
	$(CC) -o $@ $(CFLAGS) ../core/fntest/pubnub_fntest.c ../core/fntest/pubnub_fntest_basic.c ../core/fntest/pubnub_fntest_medium.c fntest/pubnub_fntest_windows.c fntest/pubnub_fntest_runner.c $(SOURCEFILES)  ../core/pubnub_ntf_sync.c $(LIBFILES)
    
clean:
	del *.exe
	del *.obj
