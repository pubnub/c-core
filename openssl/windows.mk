SOURCEFILES = ..\core\pubnub_pubsubapi.c ..\core\pubnub_coreapi.c ..\core\pubnub_ccore_pubsub.c ..\core\pubnub_ccore.c ..\core\pubnub_netcore.c ..\lib\sockets\pbpal_resolv_and_connect_sockets.c pbpal_openssl.c pbpal_connect_openssl.c pbpal_add_system_certs_windows.c ..\core\pubnub_alloc_std.c ..\core\pubnub_assert_std.c ..\core\pubnub_generate_uuid.c ..\core\pubnub_blocking_io.c ..\windows\windows_socket_blocking_io.c ..\core\pubnub_free_with_timeout_std.c ..\core\pubnub_timers.c ..\core\pubnub_json_parse.c  ..\core\pubnub_proxy.c ..\core\pubnub_proxy_core.c ..\core\pbhttp_digest.c ..\lib\md5\md5.c ..\core\pbntlm_core.c ..\core\pbntlm_packer_sspi.c ..\core\pubnub_ssl.c ..\windows\pubnub_set_proxy_from_system_windows.c  ..\core\pubnub_helper.c pubnub_version_openssl.c  ..\windows\pubnub_generate_uuid_windows.c pbpal_openssl_blocking_io.c ..\lib\base64\pbbase64.c ..\core\pubnub_crypto.c ..\core\pubnub_coreapi_ex.c pbaes256.c ..\core\c99\snprintf.c ..\core\pubnub_dns_servers.c ..\windows\pubnub_dns_system_servers.c ..\lib\pubnub_parse_ipv4_addr.c ..\lib\miniz\miniz_tinfl.c ..\lib\miniz\miniz_tdef.c ..\lib\miniz\miniz.c ..\lib\pbcrc32.c ..\core\pbgzip_compress.c ..\core\pbgzip_decompress.c ..\core\pubnub_subscribe_v2.c ..\windows\msstopwatch_windows.c ..\core\pubnub_url_encode.c ..\lib\pubnub_parse_ipv6_addr.c ..\core\pbcc_advanced_history.c ..\core\pubnub_advanced_history.c

OBJFILES = pubnub_pubsubapi.obj pubnub_coreapi.obj pubnub_ccore_pubsub.obj pubnub_ccore.obj pubnub_netcore.obj pbpal_resolv_and_connect_sockets.obj pbpal_openssl.obj pbpal_connect_openssl.obj pbpal_add_system_certs_windows.obj pubnub_alloc_std.obj pubnub_assert_std.obj pubnub_generate_uuid.obj pubnub_blocking_io.obj pubnub_free_with_timeout_std.obj pubnub_timers.obj pubnub_json_parse.obj pubnub_proxy.obj pubnub_proxy_core.obj pbhttp_digest.obj md5.obj pbntlm_core.obj pbntlm_packer_sspi.obj pubnub_ssl.obj pubnub_set_proxy_from_system_windows.obj pubnub_helper.obj pubnub_version_openssl.obj pubnub_generate_uuid_windows.obj pbpal_openssl_blocking_io.obj windows_socket_blocking_io.obj pbbase64.obj pubnub_crypto.obj pubnub_coreapi_ex.obj pbaes256.obj snprintf.obj pubnub_dns_servers.obj pubnub_dns_system_servers.obj pubnub_parse_ipv4_addr.obj miniz_tinfl.obj miniz_tdef.obj miniz.obj pbcrc32.obj pbgzip_compress.obj pbgzip_decompress.obj pubnub_subscribe_v2.obj msstopwatch_windows.obj pubnub_url_encode.obj pubnub_parse_ipv6_addr.obj pbcc_advanced_history.obj pubnub_advanced_history.obj

!ifndef OPENSSLPATH
OPENSSLPATH=c:\OpenSSL-Win32
!endif

!IF EXISTS($(OPENSSLPATH)\lib\libssl.lib)
OPENSSL_LIBS=$(OPENSSLPATH)\lib\libssl.lib $(OPENSSLPATH)\lib\libcrypto.lib
!ELSEIF EXISTS($(OPENSSLPATH)\lib\ssleay32.lib)
OPENSSL_LIBS=$(OPENSSLPATH)\lib\ssleay32.lib $(OPENSSLPATH)\lib\libeay32.lib
!ELSE
!ERROR Cannot find OpenSSL libraries, OPENSSLPATH=$(OPENSSLPATH)
!ENDIF
LIBS=ws2_32.lib IPHlpAPI.lib rpcrt4.lib $(OPENSSL_LIBS)

CFLAGS = /Zi /MP -D PUBNUB_THREADSAFE /D PUBNUB_LOG_LEVEL=PUBNUB_LOG_LEVEL_WARNING /W3 /D PUBNUB_USE_WIN_SSPI=1 /D PUBNUB_USE_IPV6=1
# /Zi enables debugging, remove to get a smaller .exe and no .pdb
# /MP uses one compiler (`cl`) process for each input file, enabling faster build
# /analyze To run the static analyzer (not compatible w/clang-cl)

INCLUDES=-I .. -I . -I ..\core\c99 -I $(OPENSSLPATH)\include

all: pubnub_sync_sample.exe pubnub_crypto_sync_sample.exe cancel_subscribe_sync_sample.exe pubnub_publish_via_post_sample.exe subscribe_publish_callback_sample.exe pubnub_callback_sample.exe pubnub_fntest.exe pubnub_console_sync.exe pubnub_console_callback.exe subscribe_publish_from_callback.exe publish_callback_subloop_sample.exe publish_queue_callback_subloop.exe

SYNC_INTF_SOURCEFILES= ..\core\pubnub_ntf_sync.c ..\core\pubnub_sync_subscribe_loop.c ..\core\srand_from_pubnub_time.c
SYNC_INTF_OBJFILES= pubnub_ntf_sync.obj pubnub_sync_subscribe_loop.obj srand_from_pubnub_time.obj

pubnub_sync.lib : $(SOURCEFILES) $(SYNC_INTF_SOURCEFILES)
	$(CC) -c $(CFLAGS) $(INCLUDES) $(SOURCEFILES) $(SYNC_INTF_SOURCEFILES)
	lib $(OBJFILES) $(SYNC_INTF_OBJFILES) -OUT:$@

CALLBACK_INTF_SOURCEFILES=pubnub_ntf_callback_windows.c pubnub_get_native_socket.c ..\core\pubnub_timer_list.c ..\lib\sockets\pbpal_ntf_callback_poller_poll.c ..\lib\sockets\pbpal_adns_sockets.c ..\lib\pubnub_dns_codec.c ..\core\pbpal_ntf_callback_queue.c ..\core\pbpal_ntf_callback_admin.c ..\core\pbpal_ntf_callback_handle_timer_list.c  ..\core\pubnub_callback_subscribe_loop.c
CALLBACK_INTF_OBJFILES=pubnub_ntf_callback_windows.obj pubnub_get_native_socket.obj pubnub_timer_list.obj pbpal_ntf_callback_poller_poll.obj pbpal_adns_sockets.obj pubnub_dns_codec.obj pbpal_ntf_callback_queue.obj pbpal_ntf_callback_admin.obj pbpal_ntf_callback_handle_timer_list.obj pubnub_callback_subscribe_loop.obj

pubnub_callback.lib : $(SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES)
	$(CC) -c $(CFLAGS) -DPUBNUB_CALLBACK_API $(INCLUDES) $(SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES)
	lib $(OBJFILES) $(CALLBACK_INTF_OBJFILES) -OUT:$@

pubnub_sync_sample.exe: ..\core\samples\pubnub_sync_sample.c pubnub_sync.lib
	$(CC) $(CFLAGS) $(INCLUDES) ..\core\samples\pubnub_sync_sample.c pubnub_sync.lib $(LIBS)

pubnub_crypto_sync_sample.exe: ..\core\samples\pubnub_crypto_sync_sample.c pubnub_sync.lib
	$(CC) $(CFLAGS) $(INCLUDES) ..\core\samples\pubnub_crypto_sync_sample.c pubnub_sync.lib $(LIBS)

cancel_subscribe_sync_sample.exe: ..\core\samples\cancel_subscribe_sync_sample.c pubnub_sync.lib
	$(CC) $(CFLAGS) $(INCLUDES) ..\core\samples\cancel_subscribe_sync_sample.c pubnub_sync.lib $(LIBS)

pubnub_publish_via_post_sample.exe: ..\core\samples\pubnub_publish_via_post_sample.c pubnub_sync.lib
	$(CC) $(CFLAGS) $(INCLUDES) ..\core\samples\pubnub_publish_via_post_sample.c pubnub_sync.lib $(LIBS)

pubnub_callback_sample.exe: ..\core\samples\pubnub_callback_sample.c pubnub_callback.lib
	$(CC) $(CFLAGS) -DPUBNUB_CALLBACK_API $(INCLUDES) ..\core\samples\pubnub_callback_sample.c  pubnub_callback.lib $(LIBS)

subscribe_publish_callback_sample.exe: ..\core\samples\subscribe_publish_callback_sample.c pubnub_callback.lib
	$(CC) $(CFLAGS) -DPUBNUB_CALLBACK_API $(INCLUDES) ..\core\samples\subscribe_publish_callback_sample.c  pubnub_callback.lib $(LIBS)

subscribe_publish_from_callback.exe: ..\core\samples\subscribe_publish_from_callback.c pubnub_callback.lib
	$(CC) $(CFLAGS) -DPUBNUB_CALLBACK_API $(INCLUDES) ..\core\samples\subscribe_publish_from_callback.c  pubnub_callback.lib  $(LIBS)

publish_callback_subloop_sample.exe: ..\core\samples\publish_callback_subloop_sample.c pubnub_callback.lib
	$(CC) $(CFLAGS) -DPUBNUB_CALLBACK_API $(INCLUDES) ..\core\samples\publish_callback_subloop_sample.c  pubnub_callback.lib  $(LIBS)

publish_queue_callback_subloop.exe: ..\core\samples\publish_queue_callback_subloop.c pubnub_callback.lib
	$(CC) $(CFLAGS) -DPUBNUB_CALLBACK_API $(INCLUDES) ..\core\samples\publish_queue_callback_subloop.c  pubnub_callback.lib  $(LIBS)

pubnub_fntest.exe: ..\core\fntest\pubnub_fntest.c ..\core\fntest\pubnub_fntest_basic.c ..\core\fntest\pubnub_fntest_medium.c  ..\windows\fntest\pubnub_fntest_windows.c ..\windows\fntest\pubnub_fntest_runner.c pubnub_sync.lib
	$(CC) $(CFLAGS) $(INCLUDES) ..\core\fntest\pubnub_fntest.c ..\core\fntest\pubnub_fntest_basic.c ..\core\fntest\pubnub_fntest_medium.c ..\windows\fntest\pubnub_fntest_windows.c ..\windows\fntest\pubnub_fntest_runner.c pubnub_sync.lib $(LIBS)

CONSOLE_SOURCEFILES=..\core\samples\console\pubnub_console.c ..\core\samples\console\pnc_helpers.c ..\core\samples\console\pnc_readers.c ..\core\samples\console\pnc_subscriptions.c

pubnub_console_sync.exe: $(CONSOLE_SOURCEFILES) ..\core\samples\console\pnc_ops_sync.c  pubnub_sync.lib
	$(CC) /Fe:$@ $(CFLAGS) /D _CRT_SECURE_NO_WARNINGS $(INCLUDES) $(CONSOLE_SOURCEFILES) ..\core\samples\console\pnc_ops_sync.c  pubnub_sync.lib $(LIBS)

pubnub_console_callback.exe: $(CONSOLE_SOURCEFILES) ..\core\samples\console\pnc_ops_callback.c pubnub_callback.lib
	$(CC) /Fe:$@ $(CFLAGS) /D _CRT_SECURE_NO_WARNINGS -D PUBNUB_CALLBACK_API $(INCLUDES) $(CONSOLE_SOURCEFILES) ..\core\samples\console\pnc_ops_callback.c pubnub_callback.lib $(LIBS)

clean:
	del *.exe
	del *.obj
	del *.pdb
	del *.il?
	del *.lib
	del *.c~
	del *.h~
	del *~
