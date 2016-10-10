SOURCEFILES = ..\core\pubnub_coreapi.c ..\core\pubnub_ccore.c ..\core\pubnub_netcore.c  ..\lib\sockets\pbpal_sockets.c ..\lib/sockets\pbpal_resolv_and_connect_sockets.c ..\core\pubnub_alloc_std.c ..\core\pubnub_assert_std.c ..\core\pubnub_generate_uuid.c ..\core\pubnub_blocking_io.c ..\core\pubnub_timers.c ..\core\pubnub_json_parse.c ..\core\pubnub_helper.c pubnub_version_windows.c  pubnub_generate_uuid_windows.c pbpal_windows_blocking_io.c ..\core\c99\snprintf.c

OBJFILES = pubnub_coreapi.obj pubnub_ccore.obj pubnub_netcore.obj pbpal_sockets.obj pbpal_resolv_and_connect_sockets.obj pubnub_alloc_std.obj pubnub_assert_std.obj pubnub_generate_uuid.obj pubnub_blocking_io.obj pubnub_timers.obj pubnub_json_parse.obj pubnub_helper.obj pubnub_version_windows.obj pubnub_generate_uuid_windows.obj pbpal_windows_blocking_io.obj snprintf.obj

CFLAGS = /Zi /MP /I ..\core /I . /I fntest /I ../core/fntest /I ..\core\c99 /W3  /D PUBNUB_THREADSAFE
# /Zi enables debugging, remove to get a smaller .exe and no .pdb
# /MP uses one compiler (`cl`) process for each input file, enabling faster build

all: pubnub_sync_sample.exe cancel_subscribe_sync_sample.exe subscribe_publish_callback_sample.exe pubnub_callback_sample.exe pubnub_fntest.exe pubnub_console_sync.exe pubnub_console_callback.exe

pubnub_sync.lib : $(SOURCEFILES) ..\core\pubnub_ntf_sync.c
	$(CC) -c $(CFLAGS) $(SOURCEFILES) ..\core\pubnub_ntf_sync.c
	lib $(OBJFILES) pubnub_ntf_sync.obj -OUT:$@
    
pubnub_callback.lib : $(SOURCEFILES) pubnub_ntf_callback_windows.c pubnub_get_native_socket.c ..\core\pubnub_timer_list.c ..\lib\sockets\pbpal_adns_sockets.c
	$(CC) -c $(CFLAGS) -DPUBNUB_CALLBACK_API $(SOURCEFILES) pubnub_ntf_callback_windows.c pubnub_get_native_socket.c ..\core\pubnub_timer_list.c ..\lib\sockets\pbpal_adns_sockets.c
	lib $(OBJFILES) pubnub_ntf_callback_windows.obj pubnub_get_native_socket.obj pubnub_timer_list.obj pbpal_adns_sockets.obj -OUT:$@

pubnub_sync_sample.exe: ..\core\samples\pubnub_sync_sample.c pubnub_sync.lib
	$(CC) $(CFLAGS) ..\core\samples\pubnub_sync_sample.c pubnub_sync.lib ws2_32.lib rpcrt4.lib
    
cancel_subscribe_sync_sample.exe: ..\core\samples\cancel_subscribe_sync_sample.c pubnub_sync.lib
	$(CC) $(CFLAGS) ..\core\samples\cancel_subscribe_sync_sample.c pubnub_sync.lib ws2_32.lib rpcrt4.lib

pubnub_callback_sample.exe: ..\core\samples\pubnub_callback_sample.c pubnub_callback.lib
	$(CC) $(CFLAGS) -DPUBNUB_CALLBACK_API -DPUBNUB_USE_ADNS=1 ..\core\samples\pubnub_callback_sample.c  pubnub_callback.lib ws2_32.lib rpcrt4.lib

subscribe_publish_callback_sample.exe: ..\core\samples\subscribe_publish_callback_sample.c pubnub_callback.lib
	$(CC) $(CFLAGS) -DPUBNUB_CALLBACK_API ..\core\samples\subscribe_publish_callback_sample.c  pubnub_callback.lib ws2_32.lib rpcrt4.lib

pubnub_fntest.exe: ..\core\fntest\pubnub_fntest.c ..\core\fntest\pubnub_fntest_basic.c ..\core\fntest\pubnub_fntest_medium.c  fntest\pubnub_fntest_windows.c fntest\pubnub_fntest_runner.c pubnub_sync.lib
	$(CC) $(CFLAGS) ..\core\fntest\pubnub_fntest.c ..\core\fntest\pubnub_fntest_basic.c ..\core\fntest\pubnub_fntest_medium.c fntest\pubnub_fntest_windows.c fntest\pubnub_fntest_runner.c pubnub_sync.lib ws2_32.lib rpcrt4.lib

CONSOLE_SOURCEFILES=..\core\samples\console\pubnub_console.c ..\core\samples\console\pnc_helpers.c ..\core\samples\console\pnc_readers.c ..\core\samples\console\pnc_subscriptions.c

pubnub_console_sync.exe: $(CONSOLE_SOURCEFILES) ..\core\samples\console\pnc_ops_sync.c  pubnub_sync.lib
	$(CC) /Fe$@ $(CFLAGS) /D _CRT_SECURE_NO_WARNINGS $(CONSOLE_SOURCEFILES) ..\core\samples\console\pnc_ops_sync.c  pubnub_sync.lib ws2_32.lib rpcrt4.lib

pubnub_console_callback.exe: $(CONSOLE_SOURCEFILES) ..\core\samples\console\pnc_ops_callback.c pubnub_callback.lib
	$(CC) /Fe$@ $(CFLAGS) /D _CRT_SECURE_NO_WARNINGS -D PUBNUB_CALLBACK_API $(CONSOLE_SOURCEFILES) ..\core\samples\console\pnc_ops_callback.c pubnub_callback.lib ws2_32.lib rpcrt4.lib

clean:
	del *.exe
	del *.obj
	del *.pdb
	del *.il?
    del *.lib
