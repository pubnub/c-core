SOURCEFILES = ..\core\pubnub_pubsubapi.c ..\core\pubnub_coreapi.c ..\core\pubnub_coreapi_ex.c ..\core\pubnub_ccore_pubsub.c ..\core\pubnub_ccore.c ..\core\pubnub_netcore.c  ..\lib\sockets\pbpal_sockets.c ..\lib/sockets\pbpal_resolv_and_connect_sockets.c ..\core\pubnub_alloc_std.c ..\core\pubnub_assert_std.c ..\core\pubnub_generate_uuid.c ..\core\pubnub_blocking_io.c ..\lib\base64\pbbase64.c ..\core\pubnub_timers.c ..\core\pubnub_json_parse.c ..\core\pubnub_proxy.c ..\core\pubnub_proxy_core.c ..\core\pbhttp_digest.c ..\lib\md5\md5.c ..\core\pbntlm_core.c ..\core\pbntlm_packer_sspi.c pubnub_set_proxy_from_system_windows.c ..\core\pubnub_helper.c pubnub_version_windows.c  pubnub_generate_uuid_windows.c pbpal_windows_blocking_io.c ..\core\c99\snprintf.c

OBJFILES = pubnub_pubsubapi.obj pubnub_coreapi.obj pubnub_coreapi_ex.obj pubnub_ccore_pubsub.obj pubnub_ccore.obj pubnub_netcore.obj pbpal_sockets.obj pbpal_resolv_and_connect_sockets.obj pubnub_alloc_std.obj pubnub_assert_std.obj pubnub_generate_uuid.obj pubnub_blocking_io.obj pbbase64.obj pubnub_timers.obj pubnub_json_parse.obj pubnub_proxy.obj pubnub_proxy_core.obj pbhttp_digest.obj md5.obj pbntlm_core.obj pbntlm_packer_sspi.obj pubnub_set_proxy_from_system_windows.obj pubnub_helper.obj pubnub_version_windows.obj pubnub_generate_uuid_windows.obj pbpal_windows_blocking_io.obj snprintf.obj

LDLIBS=ws2_32.lib rpcrt4.lib

DEFINES=-D PUBNUB_THREADSAFE -D PUBNUB_LOG_LEVEL=PUBNUB_LOG_LEVEL_WARNING -D HAVE_STRERROR_S
CFLAGS = -Zi -MP -W3 $(DEFINES)
# /Zi enables debugging, remove to get a smaller .exe and no .pdb
# /MP uses one compiler (`cl`) process for each input file, enabling faster build (ignored by clang-cl)
# /analyze To run the static analyzer (not compatible w/clang-cl)

INCLUDES=-I ..\core -I . -I ..\lib\sockets -I ..\lib\base64 -I ..\lib\md5 -I fntest -I ../core/fntest -I ..\core\c99 

all: pubnub_sync_sample.exe cancel_subscribe_sync_sample.exe subscribe_publish_callback_sample.exe pubnub_callback_sample.exe pubnub_callback_subloop_sample.exe pubnub_fntest.exe pubnub_console_sync.exe pubnub_console_callback.exe

SYNC_INTF_SOURCEFILES= ..\core\pubnub_ntf_sync.c

pubnub_sync.lib : $(SOURCEFILES) $(SYNC_INTF_SOURCEFILES)
	$(CC) -c $(CFLAGS) $(INCLUDES) $(SOURCEFILES) $(SYNC_INTF_SOURCEFILES)
	lib $(OBJFILES) pubnub_ntf_sync.obj -OUT:$@

CALLBACK_INTF_SOURCEFILES=pubnub_ntf_callback_windows.c pubnub_get_native_socket.c ..\core\pubnub_timer_list.c ..\lib\sockets\pbpal_adns_sockets.c ..\lib\sockets\pbpal_ntf_callback_poller_poll.c  ..\core\pbpal_ntf_callback_queue.c  ..\core\pubnub_callback_subscribe_loop.c
CALLBACK_INTF_OBJFILES=pubnub_ntf_callback_windows.obj pubnub_get_native_socket.obj pubnub_timer_list.obj pbpal_adns_sockets.obj pbpal_ntf_callback_poller_poll.obj pbpal_ntf_callback_queue.obj pubnub_callback_subscribe_loop.obj

pubnub_callback.lib : $(SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES)
	$(CC) -c $(CFLAGS) -DPUBNUB_CALLBACK_API $(INCLUDES) $(SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES)
	lib $(OBJFILES) $(CALLBACK_INTF_OBJFILES) -OUT:$@

SYNC_SAMPLE_SOURCEFILES= ..\core\samples\pubnub_sync_sample.c

pubnub_sync_sample.exe: $(SYNC_SAMPLE_SOURCEFILES)  pubnub_sync.lib
	$(CC) $(CFLAGS) $(INCLUDES) $(SYNC_SAMPLE_SOURCEFILES) pubnub_sync.lib $(LDLIBS)

cancel_subscribe_sync_sample.exe: ..\core\samples\cancel_subscribe_sync_sample.c pubnub_sync.lib
	$(CC) $(CFLAGS) $(INCLUDES) ..\core\samples\cancel_subscribe_sync_sample.c pubnub_sync.lib  $(LDLIBS)

pubnub_callback_sample.exe: ..\core\samples\pubnub_callback_sample.c pubnub_callback.lib
	$(CC) $(CFLAGS) -DPUBNUB_CALLBACK_API -DPUBNUB_USE_ADNS=1 $(INCLUDES) ..\core\samples\pubnub_callback_sample.c  pubnub_callback.lib  $(LDLIBS)

subscribe_publish_callback_sample.exe: ..\core\samples\subscribe_publish_callback_sample.c pubnub_callback.lib
	$(CC) $(CFLAGS) -DPUBNUB_CALLBACK_API $(INCLUDES) ..\core\samples\subscribe_publish_callback_sample.c  pubnub_callback.lib  $(LDLIBS)

pubnub_callback_subloop_sample.exe: ..\core\samples\pubnub_callback_subloop_sample.c ..\core\pubnub_create.c pubnub_callback.lib
	$(CC) -D PUBNUB_CALLBACK_API $(CFLAGS) $(INCLUDES) ..\core\samples\pubnub_callback_subloop_sample.c ..\core\pubnub_create.c pubnub_callback.lib $(LDLIBS)

pubnub_fntest.exe: ..\core\fntest\pubnub_fntest.c ..\core\fntest\pubnub_fntest_basic.c ..\core\fntest\pubnub_fntest_medium.c  fntest\pubnub_fntest_windows.c fntest\pubnub_fntest_runner.c pubnub_sync.lib
	$(CC) $(CFLAGS) $(INCLUDES) ..\core\fntest\pubnub_fntest.c ..\core\fntest\pubnub_fntest_basic.c ..\core\fntest\pubnub_fntest_medium.c fntest\pubnub_fntest_windows.c fntest\pubnub_fntest_runner.c pubnub_sync.lib  $(LDLIBS)

CONSOLE_SOURCEFILES=..\core\samples\console\pubnub_console.c ..\core\samples\console\pnc_helpers.c ..\core\samples\console\pnc_readers.c ..\core\samples\console\pnc_subscriptions.c

pubnub_console_sync.exe: $(CONSOLE_SOURCEFILES) ..\core\samples\console\pnc_ops_sync.c  pubnub_sync.lib
	$(CC) /Fe$@ $(CFLAGS) /D _CRT_SECURE_NO_WARNINGS $(INCLUDES) $(CONSOLE_SOURCEFILES) ..\core\samples\console\pnc_ops_sync.c  pubnub_sync.lib  $(LDLIBS)

pubnub_console_callback.exe: $(CONSOLE_SOURCEFILES) ..\core\samples\console\pnc_ops_callback.c pubnub_callback.lib
	$(CC) /Fe$@ $(CFLAGS) /D _CRT_SECURE_NO_WARNINGS -D PUBNUB_CALLBACK_API $(INCLUDES) $(CONSOLE_SOURCEFILES) ..\core\samples\console\pnc_ops_callback.c pubnub_callback.lib  $(LDLIBS)

cppcheck: $(SOURCEFILES) $(CONSOLE_SOURCEFILES) $(SYNC_INTF_SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES) $(SYNC_SAMPLE_SOURCEFILES)
	cppcheck $(INCLUDES) $(DEFINES) --force --enable=all $(SOURCEFILES) $(CONSOLE_SOURCEFILES) $(SYNC_INTF_SOURCEFILES)  $(CALLBACK_INTF_SOURCEFILES) $(SYNC_SAMPLE_SOURCEFILES)

clean:
	del *.exe
	del *.obj
	del *.pdb
	del *.il?
	del *.lib
