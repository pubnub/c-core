SOURCEFILES = ..\core\pubnub_pubsubapi.c ..\core\pubnub_coreapi.c  ..\core\pubnub_coreapi_ex.c  ..\core\pubnub_ccore_pubsub.c ..\core\pubnub_ccore.c ..\core\pubnub_netcore.c  ..\lib\sockets\pbpal_sockets.c ..\lib\sockets\pbpal_resolv_and_connect_sockets.c ..\core\pubnub_alloc_std.c ..\core\pubnub_assert_std.c ..\core\pubnub_generate_uuid.c ..\core\pubnub_timers.c ..\core\pubnub_blocking_io.c  ..\lib\base64\pbbase64.c ..\core\pubnub_json_parse.c ..\core\pubnub_free_with_timeout_std.c ..\core\pubnub_proxy.c ..\core\pubnub_proxy_core.c ..\core\pbhttp_digest.c ..\lib\md5\md5.c ..\core\pbntlm_core.c ..\core\pbntlm_packer_sspi.c ..\windows\pubnub_set_proxy_from_system_windows.c ..\core\pubnub_helper.c  ..\windows\pubnub_version_windows.c ..\windows\pubnub_generate_uuid_windows.c ..\windows\pbpal_windows_blocking_io.c ..\windows\windows_socket_blocking_io.c ..\core\c99\snprintf.c ..\core\pubnub_dns_servers.c ..\windows\pubnub_dns_system_servers.c ..\lib\pubnub_parse_ipv4_addr.c ..\lib\miniz\miniz_tinfl.c ..\core\pbgzip_decompress.c  ..\core\pubnub_subscribe_v2.c  ..\windows\msstopwatch_windows.c

LIBS=ws2_32.lib rpcrt4.lib

CFLAGS = /EHsc /Zi /MP /TP /I .. /I . /I ..\core\c99 /I ..\windows /W3 /D PUBNUB_THREADSAFE /D HAVE_STRERROR_S
# /Zi enables debugging, remove to get a smaller .exe and no .pdb 
# /MP uses one compiler (`cl`) process for each input file, enabling faster build
# /TP means "compile all files as C++"
# /EHsc enables (standard) exception support
# /analyze runs static analysis

all: fntest_runner.exe pubnub_sync_sample.exe pubnub_callback_sample.exe pubnub_callback_cpp11_sample.exe cancel_subscribe_sync_sample.exe subscribe_publish_callback_sample.exe futres_nesting_sync.exe futres_nesting_callback.exe futres_nesting_callback_cpp11.exe


pubnub_sync_sample.exe: samples\pubnub_sample.cpp $(SOURCEFILES) ..\core\pubnub_ntf_sync.c pubnub_futres_sync.cpp
	$(CXX) /Fe$@ $(CFLAGS) samples\pubnub_sample.cpp ..\core\pubnub_ntf_sync.c pubnub_futres_sync.cpp $(SOURCEFILES) /link $(LIBS)

cancel_subscribe_sync_sample.exe: samples\cancel_subscribe_sync_sample.cpp $(SOURCEFILES) ..\core\pubnub_ntf_sync.c pubnub_futres_sync.cpp
	$(CXX) /Fe$@ $(CFLAGS) samples\cancel_subscribe_sync_sample.cpp ..\core\pubnub_ntf_sync.c pubnub_futres_sync.cpp $(SOURCEFILES) /link $(LIBS)

futres_nesting_sync.exe: samples\futres_nesting.cpp $(SOURCEFILES) ..\core\pubnub_ntf_sync.c pubnub_futres_sync.cpp
	$(CXX) /Fe$@ $(CFLAGS) samples\futres_nesting.cpp ..\core\pubnub_ntf_sync.c pubnub_futres_sync.cpp $(SOURCEFILES) /link $(LIBS)

fntest_runner.exe: fntest\pubnub_fntest_runner.cpp $(SOURCEFILES)  ..\core\pubnub_ntf_sync.c pubnub_futres_sync.cpp fntest\pubnub_fntest.cpp fntest\pubnub_fntest_basic.cpp fntest\pubnub_fntest_medium.cpp
	$(CXX) /Fe$@ $(CFLAGS) fntest\pubnub_fntest_runner.cpp ..\core\pubnub_ntf_sync.c pubnub_futres_sync.cpp fntest/pubnub_fntest.cpp fntest\pubnub_fntest_basic.cpp fntest\pubnub_fntest_medium.cpp $(SOURCEFILES) /link $(LIBS) 


##
# The socket poller module to use. You should use the `poll` poller it
# doesn't have the weird restrictions of `select` poller. OTOH,
# select() on Windows is compatible w/BSD sockets poll(), while
# WSAPoll() has some weird differences to poll().  The names are the
# same until the last `_`, then it's `poll` vs `select.
SOCKET_POLLER_C=..\lib\sockets\pbpal_ntf_callback_poller_poll.c
SOCKET_POLLER_OBJ=pbpal_ntf_callback_poller_poll.obj

CALLBACK_INTF_SOURCEFILES=..\windows\pubnub_ntf_callback_windows.c ..\windows\pubnub_get_native_socket.c ..\core\pubnub_timer_list.c ..\lib\sockets\pbpal_adns_sockets.c ..\lib\pubnub_dns_codec.c $(SOCKET_POLLER_C)  ..\core\pbpal_ntf_callback_queue.c ..\core\pbpal_ntf_callback_admin.c ..\core\pbpal_ntf_callback_handle_timer_list.c  ..\core\pubnub_callback_subscribe_loop.c
CALLBACK_INTF_OBJFILES=pubnub_ntf_callback_windows.obj pubnub_get_native_socket.obj pubnub_timer_list.obj pbpal_adns_sockets.obj pubnub_dns_codec.obj $(SOCKET_POLLER_OBJ) pbpal_ntf_callback_queue.obj pbpal_ntf_callback_admin.obj pbpal_ntf_callback_handle_timer_list.obj pubnub_callback_subscribe_loop.obj


pubnub_callback_sample.exe: samples\pubnub_sample.cpp $(SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_windows.cpp
	$(CXX) /Fe$@ -D PUBNUB_CALLBACK_API $(CFLAGS) samples\pubnub_sample.cpp $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_windows.cpp $(SOURCEFILES) /link $(LIBS)

pubnub_callback_cpp11_sample.exe: samples\pubnub_sample.cpp $(SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_cpp11.cpp
	$(CXX) /Fe$@ /D PUBNUB_CALLBACK_API $(CFLAGS) samples\pubnub_sample.cpp $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_cpp11.cpp $(SOURCEFILES) /link $(LIBS)

subscribe_publish_callback_sample.exe: samples\subscribe_publish_callback_sample.cpp $(SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_windows.cpp
	$(CXX) /Fe$@ -D PUBNUB_CALLBACK_API $(CFLAGS) samples\subscribe_publish_callback_sample.cpp $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_windows.cpp $(SOURCEFILES) /link $(LIBS)

futres_nesting_callback.exe: samples\futres_nesting.cpp $(SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_windows.cpp
	$(CXX) /Fe$@ -D PUBNUB_CALLBACK_API $(CFLAGS) samples\futres_nesting.cpp $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_windows.cpp $(SOURCEFILES) /link $(LIBS)

futres_nesting_callback_cpp11.exe: samples\futres_nesting.cpp $(SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_cpp11.cpp
	$(CXX) /Fe$@ -D PUBNUB_CALLBACK_API $(CFLAGS) samples\futres_nesting.cpp $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_cpp11.cpp $(SOURCEFILES) /link $(LIBS)


clean:
	del *.obj
	del *.exe
	del *.pdb
	del *.il?

