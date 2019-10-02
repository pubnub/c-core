SOURCEFILES = ..\core\pubnub_pubsubapi.c ..\core\pubnub_coreapi.c ..\core\pubnub_coreapi_ex.c ..\core\pubnub_ccore_pubsub.c ..\core\pubnub_ccore.c ..\core\pubnub_netcore.c ..\lib\sockets\pbpal_sockets.c ..\lib\sockets\pbpal_resolv_and_connect_sockets.c ..\lib\sockets\pbpal_handle_socket_error.c ..\core\pubnub_alloc_std.c ..\core\pubnub_assert_std.c ..\core\pubnub_generate_uuid.c ..\core\pubnub_timers.c ..\core\pubnub_blocking_io.c ..\lib\base64\pbbase64.c ..\core\pubnub_json_parse.c ..\core\pubnub_free_with_timeout_std.c ..\lib\md5\md5.c ..\lib\pb_strnlen_s.c ..\core\pubnub_helper.c pubnub_version_windows.cpp ..\windows\pubnub_generate_uuid_windows.c ..\windows\pbpal_windows_blocking_io.c ..\windows\windows_socket_blocking_io.c ..\core\c99\snprintf.c ..\lib\miniz\miniz_tinfl.c ..\lib\miniz\miniz_tdef.c ..\lib\miniz\miniz.c ..\lib\pbcrc32.c ..\core\pbgzip_compress.c ..\core\pbgzip_decompress.c ..\core\pbcc_subscribe_v2.c ..\core\pubnub_subscribe_v2.c ..\windows\msstopwatch_windows.c ..\core\pubnub_url_encode.c ..\core\pbcc_advanced_history.c ..\core\pubnub_advanced_history.c ..\core\pbcc_objects_api.c ..\core\pubnub_objects_api.c ..\core\pbcc_actions_api.c ..\core\pubnub_actions_api.c

LIBS=ws2_32.lib rpcrt4.lib

!ifndef ONLY_PUBSUB_API
ONLY_PUBSUB_API = 0
!endif

!ifndef USE_PROXY
USE_PROXY = 1
!endif

!if $(USE_PROXY)
PROXY_INTF_SOURCEFILES = ..\core\pubnub_proxy.c ..\core\pubnub_proxy_core.c ..\core\pbhttp_digest.c ..\core\pbntlm_core.c ..\core\pbntlm_packer_sspi.c ..\windows\pubnub_set_proxy_from_system_windows.c 
PROXY_INTF_OBJFILES = pubnub_proxy.obj pubnub_proxy_core.obj pbhttp_digest.obj pbntlm_core.obj pbntlm_packer_sspi.obj pubnub_set_proxy_from_system_windows.obj 
!endif

CFLAGS = /EHsc /Zi /MP /TP /I .. /I . /I ..\core\c99 /I ..\windows /W3 /D PUBNUB_THREADSAFE /D HAVE_STRERROR_S /D PUBNUB_ONLY_PUBSUB_API=$(ONLY_PUBSUB_API) /D PUBNUB_PROXY_API=$(USE_PROXY)
# /Zi enables debugging, remove to get a smaller .exe and no .pdb 
# /MP uses one compiler (`cl`) process for each input file, enabling faster build
# /TP means "compile all files as C++"
# /EHsc enables (standard) exception support
# /analyze runs static analysis

all: fntest_runner.exe pubnub_sync_sample.exe pubnub_callback_sample.exe pubnub_callback_cpp11_sample.exe cancel_subscribe_sync_sample.exe subscribe_publish_callback_sample.exe futres_nesting_sync.exe futres_nesting_callback.exe futres_nesting_callback_cpp11.exe


pubnub_sync_sample.exe: samples\pubnub_sample.cpp $(SOURCEFILES) $(PROXY_INTF_SOURCEFILES) ..\core\pubnub_ntf_sync.c pubnub_futres_sync.cpp
	$(CXX) /Fe$@ $(CFLAGS) samples\pubnub_sample.cpp ..\core\pubnub_ntf_sync.c pubnub_futres_sync.cpp $(SOURCEFILES) $(PROXY_INTF_SOURCEFILES) /link $(LIBS)

cancel_subscribe_sync_sample.exe: samples\cancel_subscribe_sync_sample.cpp $(SOURCEFILES) $(PROXY_INTF_SOURCEFILES) ..\core\pubnub_ntf_sync.c pubnub_futres_sync.cpp
	$(CXX) /Fe$@ $(CFLAGS) samples\cancel_subscribe_sync_sample.cpp ..\core\pubnub_ntf_sync.c pubnub_futres_sync.cpp $(SOURCEFILES) $(PROXY_INTF_SOURCEFILES) /link $(LIBS)

futres_nesting_sync.exe: samples\futres_nesting.cpp $(SOURCEFILES) $(PROXY_INTF_SOURCEFILES) ..\core\pubnub_ntf_sync.c pubnub_futres_sync.cpp
	$(CXX) /Fe$@ $(CFLAGS) samples\futres_nesting.cpp ..\core\pubnub_ntf_sync.c pubnub_futres_sync.cpp $(SOURCEFILES) $(PROXY_INTF_SOURCEFILES) /link $(LIBS)

fntest_runner.exe: fntest\pubnub_fntest_runner.cpp $(SOURCEFILES) $(PROXY_INTF_SOURCEFILES) ..\core\pubnub_ntf_sync.c ..\core\srand_from_pubnub_time.c pubnub_futres_sync.cpp fntest\pubnub_fntest.cpp fntest\pubnub_fntest_basic.cpp fntest\pubnub_fntest_medium.cpp
	$(CXX) /Fe$@ $(CFLAGS) fntest\pubnub_fntest_runner.cpp ..\core\pubnub_ntf_sync.c ..\core\srand_from_pubnub_time.c pubnub_futres_sync.cpp fntest/pubnub_fntest.cpp fntest\pubnub_fntest_basic.cpp fntest\pubnub_fntest_medium.cpp $(SOURCEFILES) $(PROXY_INTF_SOURCEFILES) /link $(LIBS) 


##
# The socket poller module to use. You should use the `poll` poller it
# doesn't have the weird restrictions of `select` poller. OTOH,
# select() on Windows is compatible w/BSD sockets poll(), while
# WSAPoll() has some weird differences to poll().  The names are the
# same until the last `_`, then it's `poll` vs `select.
SOCKET_POLLER_C=..\lib\sockets\pbpal_ntf_callback_poller_poll.c
SOCKET_POLLER_OBJ=pbpal_ntf_callback_poller_poll.obj

CALLBACK_INTF_SOURCEFILES=..\windows\pubnub_ntf_callback_windows.c ..\windows\pubnub_get_native_socket.c ..\core\pubnub_timer_list.c ..\lib\sockets\pbpal_adns_sockets.c ..\lib\pubnub_dns_codec.c ..\core\pubnub_dns_servers.c ..\windows\pubnub_dns_system_servers.c ..\lib\pubnub_parse_ipv4_addr.c ..\lib\pubnub_parse_ipv6_addr.c $(SOCKET_POLLER_C) ..\core\pbpal_ntf_callback_queue.c ..\core\pbpal_ntf_callback_admin.c ..\core\pbpal_ntf_callback_handle_timer_list.c ..\core\pubnub_callback_subscribe_loop.c
CALLBACK_INTF_OBJFILES=pubnub_ntf_callback_windows.obj pubnub_get_native_socket.obj pubnub_timer_list.obj pbpal_adns_sockets.obj pubnub_dns_codec.obj pubnub_dns_servers.obj pubnub_dns_system_servers.obj pubnub_parse_ipv4_addr.obj pubnub_parse_ipv6_addr.obj $(SOCKET_POLLER_OBJ) pbpal_ntf_callback_queue.obj pbpal_ntf_callback_admin.obj pbpal_ntf_callback_handle_timer_list.obj pubnub_callback_subscribe_loop.obj


pubnub_callback_sample.exe: samples\pubnub_sample.cpp $(SOURCEFILES) $(PROXY_INTF_SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_windows.cpp
	$(CXX) /Fe$@ -D PUBNUB_CALLBACK_API $(CFLAGS) samples\pubnub_sample.cpp $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_windows.cpp $(SOURCEFILES) $(PROXY_INTF_SOURCEFILES) /link $(LIBS)

pubnub_callback_cpp11_sample.exe: samples\pubnub_sample.cpp $(SOURCEFILES) $(PROXY_INTF_SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_cpp11.cpp
	$(CXX) /Fe$@ /D PUBNUB_CALLBACK_API $(CFLAGS) samples\pubnub_sample.cpp $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_cpp11.cpp $(SOURCEFILES) $(PROXY_INTF_SOURCEFILES) /link $(LIBS)

subscribe_publish_callback_sample.exe: samples\subscribe_publish_callback_sample.cpp $(SOURCEFILES) $(PROXY_INTF_SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_windows.cpp
	$(CXX) /Fe$@ -D PUBNUB_CALLBACK_API $(CFLAGS) samples\subscribe_publish_callback_sample.cpp $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_windows.cpp $(SOURCEFILES) $(PROXY_INTF_SOURCEFILES) /link $(LIBS)

futres_nesting_callback.exe: samples\futres_nesting.cpp $(SOURCEFILES) $(PROXY_INTF_SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_windows.cpp
	$(CXX) /Fe$@ -D PUBNUB_CALLBACK_API $(CFLAGS) samples\futres_nesting.cpp $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_windows.cpp $(SOURCEFILES) $(PROXY_INTF_SOURCEFILES) /link $(LIBS)

futres_nesting_callback_cpp11.exe: samples\futres_nesting.cpp $(SOURCEFILES) $(PROXY_INTF_SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_cpp11.cpp
	$(CXX) /Fe$@ -D PUBNUB_CALLBACK_API $(CFLAGS) samples\futres_nesting.cpp $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_cpp11.cpp $(SOURCEFILES) $(PROXY_INTF_SOURCEFILES) /link $(LIBS)


clean:
	del *.obj
	del *.exe
	del *.pdb
	del *.il?

