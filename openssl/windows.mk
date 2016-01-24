SOURCEFILES = ..\core\pubnub_coreapi.c ..\core\pubnub_ccore.c ..\core\pubnub_netcore.c  pbpal_openssl.c pbpal_resolv_and_connect_openssl.c ..\core\pubnub_alloc_std.c ..\core\pubnub_assert_std.c ..\core\pubnub_generate_uuid.c ..\core\pubnub_blocking_io.c ..\core\pubnub_timers.c ..\core\pubnub_json_parse.c ..\core\pubnub_helper.c pubnub_version_openssl.c  ..\windows\pubnub_generate_uuid_windows.c pbpal_openssl_blocking_io.c ..\core\c99\snprintf.c

!ifndef OPENSSLPATH
OPENSSLPATH=c:\OpenSSL-Win32
!endif

LIBS=ws2_32.lib rpcrt4.lib $(OPENSSLPATH)\lib\ssleay32.lib $(OPENSSLPATH)\lib\libeay32.lib

CFLAGS = /Zi /MP /D VERBOSE_DEBUG /I ..\core /I . /I ..\windows\fntest /I ..\core\fntest /I ..\core\c99 /I $(OPENSSLPATH)\include /W3
# /Zi enables debugging, remove to get a smaller .exe and no .pdb
# /MP uses one compiler (`cl`) process for each input file, enabling faster build


all: pubnub_sync_sample.exe cancel_subscribe_sync_sample.exe subscribe_publish_callback_sample.exe pubnub_callback_sample.exe pubnub_fntest.exe

pubnub_sync_sample.exe: ..\core\samples\pubnub_sync_sample.c $(SOURCEFILES) ..\core\pubnub_ntf_sync.c
	$(CC) $(CFLAGS) ..\core\samples\pubnub_sync_sample.c $(SOURCEFILES) ..\core\pubnub_ntf_sync.c $(LIBS)

cancel_subscribe_sync_sample.exe: ..\core\samples\cancel_subscribe_sync_sample.c $(SOURCEFILES) ..\core\pubnub_ntf_sync.c
	$(CC) $(CFLAGS) ..\core\samples\cancel_subscribe_sync_sample.c $(SOURCEFILES) ..\core\pubnub_ntf_sync.c $(LIBS)

pubnub_callback_sample.exe: ..\core\samples\pubnub_callback_sample.c $(SOURCEFILES) ..\core\pubnub_timer_list.c pubnub_ntf_callback_windows.c pubnub_get_native_socket.c
	$(CC) $(CFLAGS) -DPUBNUB_CALLBACK_API ..\core\samples\pubnub_callback_sample.c  $(SOURCEFILES)  ..\core\pubnub_timer_list.c pubnub_ntf_callback_windows.c  pubnub_get_native_socket.c $(LIBS)

subscribe_publish_callback_sample.exe: ..\core\samples\subscribe_publish_callback_sample.c $(SOURCEFILES)  ..\core\pubnub_timer_list.c pubnub_ntf_callback_windows.c  pubnub_get_native_socket.c
	$(CC) $(CFLAGS) -DPUBNUB_CALLBACK_API ..\core\samples\subscribe_publish_callback_sample.c  $(SOURCEFILES)  ..\core\pubnub_timer_list.c pubnub_ntf_callback_windows.c  pubnub_get_native_socket.c $(LIBS)

pubnub_fntest.exe: ..\core\fntest\pubnub_fntest.c ..\core\fntest\pubnub_fntest_basic.c ..\core\fntest\pubnub_fntest_medium.c  ..\windows\fntest\pubnub_fntest_windows.c ..\windows\fntest\pubnub_fntest_runner.c $(SOURCEFILES)  ..\core\pubnub_ntf_sync.c
	$(CC) $(CFLAGS) ..\core\fntest\pubnub_fntest.c ..\core\fntest\pubnub_fntest_basic.c ..\core\fntest\pubnub_fntest_medium.c ..\windows\fntest\pubnub_fntest_windows.c ..\windows\fntest\pubnub_fntest_runner.c $(SOURCEFILES)  ..\core\pubnub_ntf_sync.c $(LIBS)

clean:
	del *.exe
	del *.obj
	del *.pdb
	del *.il?
