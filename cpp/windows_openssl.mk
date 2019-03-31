SOURCEFILES = ..\core\pubnub_pubsubapi.c ..\core\pubnub_coreapi.c ..\core\pubnub_ccore_pubsub.c ..\core\pubnub_ccore.c ..\core\pubnub_netcore.c ..\lib\sockets\pbpal_resolv_and_connect_sockets.c ..\openssl\pbpal_openssl.c ..\openssl\pbpal_connect_openssl.c ..\core\pubnub_alloc_std.c ..\core\pubnub_assert_std.c ..\core\pubnub_generate_uuid.c ..\core\pubnub_blocking_io.c ..\lib\base64\pbbase64.c ..\core\pubnub_json_parse.c ..\core\pubnub_proxy.c ..\core\pubnub_proxy_core.c ..\core\pbhttp_digest.c ..\core\pubnub_helper.c ..\openssl\pubnub_version_openssl.c ..\windows\pubnub_generate_uuid_windows.c ..\openssl\pbpal_openssl_blocking_io.c ..\windows\windows_socket_blocking_io.c ..\core\pubnub_timers.c ..\core\c99\snprintf.c ..\openssl\pbpal_add_system_certs_windows.c ..\core\pubnub_free_with_timeout_std.c ..\lib\md5\md5.c ..\core\pbntlm_core.c ..\core\pbntlm_packer_sspi.c ..\core\pubnub_ssl.c ..\windows\pubnub_set_proxy_from_system_windows.c ..\core\pubnub_crypto.c ..\core\pubnub_coreapi_ex.c ..\openssl\pbaes256.c ..\core\pubnub_dns_servers.c ..\windows\pubnub_dns_system_servers.c ..\lib\pubnub_parse_ipv4_addr.c ..\lib\miniz\miniz_tinfl.c ..\lib\miniz\miniz_tdef.c ..\lib\miniz\miniz.c ..\lib\pbcrc32.c ..\core\pbgzip_compress.c ..\core\pbgzip_decompress.c  ..\core\pubnub_subscribe_v2.c  ..\windows\msstopwatch_windows.c ..\core\pubnub_url_encode.c ..\lib\pubnub_parse_ipv6_addr.c ..\core\pbcc_advanced_history.c ..\core\pubnub_advanced_history.c

!ifndef OPENSSLPATH
OPENSSLPATH=c:\OpenSSL-Win32
!endif

!IF EXISTS($(OPENSSLPATH)\lib\libssl.lib)
OPENSSL_LIBS=$(OPENSSLPATH)\lib\libssl.lib $(OPENSSLPATH)\lib\libcrypto.lib
!ELSEIF EXISTS($(OPENSSLPATH)\lib\ssleay32.lib)
OPENSSL_LIBS=$(OPENSSLPATH)\lib\ssleay32.lib $(OPENSSLPATH)\lib\libeay32.lib
!ELSE
!ERROR Cannot find OpenSSL libraries
!ENDIF
LIBS=ws2_32.lib rpcrt4.lib $(OPENSSL_LIBS)

INCLUDES=-I .. -I . -I ..\core\c99 -I ..\openssl -I $(OPENSSLPATH)\include

CFLAGS = /EHsc /Zi /MP /TP /W3 $(INCLUDES) /D PUBNUB_THREADSAFE /D PUBNUB_USE_WIN_SSPI=1 /D PUBNUB_USE_IPV6=1
# /Zi enables debugging, remove to get a smaller .exe and no .pdb 
# /MP uses one compiler (`cl`) process for each input file, enabling faster build
# /TP means "compile all files as C++"
# /EHsc enables (standard) exception support

all: openssl\fntest_runner.exe openssl\pubnub_sync_sample.exe openssl\pubnub_callback_sample.exe openssl\pubnub_callback_cpp11_sample.exe openssl\cancel_subscribe_sync_sample.exe openssl\subscribe_publish_callback_sample.exe openssl\futres_nesting_sync.exe openssl\futres_nesting_callback.exe openssl\futres_nesting_callback_cpp11.exe


openssl\pubnub_sync_sample.exe: samples\pubnub_sample.cpp $(SOURCEFILES) ..\core\pubnub_ntf_sync.c pubnub_futres_sync.cpp
	$(CXX) /Fe$@ $(CFLAGS) samples\pubnub_sample.cpp ..\core\pubnub_ntf_sync.c pubnub_futres_sync.cpp $(SOURCEFILES) /link $(LIBS)

openssl\cancel_subscribe_sync_sample.exe: samples\cancel_subscribe_sync_sample.cpp $(SOURCEFILES) ..\core\pubnub_ntf_sync.c pubnub_futres_sync.cpp
	$(CXX) /Fe$@ $(CFLAGS) samples\cancel_subscribe_sync_sample.cpp ..\core\pubnub_ntf_sync.c pubnub_futres_sync.cpp $(SOURCEFILES) /link $(LIBS)

openssl\futres_nesting_sync.exe: samples\futres_nesting.cpp $(SOURCEFILES) ..\core\pubnub_ntf_sync.c pubnub_futres_sync.cpp
	$(CXX) /Fe$@ $(CFLAGS) samples\futres_nesting.cpp ..\core\pubnub_ntf_sync.c pubnub_futres_sync.cpp $(SOURCEFILES) /link $(LIBS)

openssl\fntest_runner.exe: fntest\pubnub_fntest_runner.cpp $(SOURCEFILES)  ..\core\pubnub_ntf_sync.c ..\core\srand_from_pubnub_time.c pubnub_futres_sync.cpp fntest\pubnub_fntest.cpp fntest\pubnub_fntest_basic.cpp fntest\pubnub_fntest_medium.cpp
	$(CXX) /Fe$@ $(CFLAGS) fntest\pubnub_fntest_runner.cpp ..\core\pubnub_ntf_sync.c ..\core\srand_from_pubnub_time.c pubnub_futres_sync.cpp fntest/pubnub_fntest.cpp fntest\pubnub_fntest_basic.cpp fntest\pubnub_fntest_medium.cpp $(SOURCEFILES) /link $(LIBS) 

CALLBACK_INTF_SOURCEFILES=..\openssl\pubnub_ntf_callback_windows.c ..\openssl\pubnub_get_native_socket.c ..\core\pubnub_timer_list.c ..\lib\sockets\pbpal_adns_sockets.c ..\lib\pubnub_dns_codec.c ..\lib\sockets\pbpal_ntf_callback_poller_poll.c  ..\core\pbpal_ntf_callback_queue.c ..\core\pbpal_ntf_callback_admin.c ..\core\pbpal_ntf_callback_handle_timer_list.c  ..\core\pubnub_callback_subscribe_loop.c

openssl\pubnub_callback_sample.exe: samples\pubnub_sample.cpp $(SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_windows.cpp
	$(CXX) /Fe$@ -D PUBNUB_CALLBACK_API $(CFLAGS) samples\pubnub_sample.cpp $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_windows.cpp $(SOURCEFILES) /link $(LIBS)

openssl\pubnub_callback_cpp11_sample.exe: samples\pubnub_sample.cpp $(SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_cpp11.cpp
	$(CXX) /Fe$@ /D PUBNUB_CALLBACK_API $(CFLAGS) samples\pubnub_sample.cpp $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_cpp11.cpp $(SOURCEFILES) /link $(LIBS)

openssl\subscribe_publish_callback_sample.exe: samples\subscribe_publish_callback_sample.cpp $(SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_windows.cpp
	$(CXX) /Fe$@ -D PUBNUB_CALLBACK_API $(CFLAGS) samples\subscribe_publish_callback_sample.cpp $(SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_windows.cpp  /link $(LIBS)

openssl\futres_nesting_callback.exe: samples\futres_nesting.cpp $(SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_windows.cpp
	$(CXX) /Fe$@ -D PUBNUB_CALLBACK_API $(CFLAGS)  samples\futres_nesting.cpp $(SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_windows.cpp /link $(LIBS)

openssl\futres_nesting_callback_cpp11.exe: samples\futres_nesting.cpp $(SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_cpp11.cpp
	$(CXX) /Fe$@ -D PUBNUB_CALLBACK_API $(CFLAGS)  samples\futres_nesting.cpp $(SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_cpp11.cpp /link $(LIBS)


clean:
	del openssl\*.obj
	del openssl\*.exe
	del openssl\*.pdb
	del openssl\*.il?
