SOURCEFILES = ..\core\pbcc_set_state.c ..\core\pubnub_pubsubapi.c ..\core\pubnub_coreapi.c ..\core\pubnub_coreapi_ex.c ..\core\pubnub_ccore_pubsub.c ..\core\pubnub_ccore.c ..\core\pubnub_netcore.c ..\lib\sockets\pbpal_sockets.c ..\lib\sockets\pbpal_resolv_and_connect_sockets.c ..\lib\sockets\pbpal_handle_socket_error.c ..\core\pubnub_alloc_std.c ..\core\pubnub_assert_std.c ..\core\pubnub_generate_uuid.c ..\core\pubnub_blocking_io.c ..\windows\windows_socket_blocking_io.c ..\core\pubnub_free_with_timeout_std.c pbtimespec_elapsed_ms.c ..\lib\base64\pbbase64.c ..\core\pubnub_timers.c ..\core\pubnub_json_parse.c ..\lib\md5\md5.c ..\lib\pb_strnlen_s.c ..\lib\pb_strncasecmp.c ..\core\pubnub_helper.c pubnub_version_windows.c  pubnub_generate_uuid_windows.c pbpal_windows_blocking_io.c ..\core\c99\snprintf.c ..\lib\miniz\miniz_tinfl.c ..\lib\miniz\miniz_tdef.c ..\lib\miniz\miniz.c ..\lib\pbcrc32.c ..\core\pbgzip_compress.c ..\core\pbgzip_decompress.c ..\core\pbcc_subscribe_v2.c ..\core\pubnub_subscribe_v2.c msstopwatch_windows.c ..\core\pubnub_url_encode.c ..\core\pbcc_advanced_history.c ..\core\pubnub_advanced_history.c ..\core\pbcc_objects_api.c ..\core\pubnub_objects_api.c ..\core\pbcc_actions_api.c ..\core\pubnub_actions_api.c ..\core\pubnub_memory_block.c ..\lib\pbstr_remove_from_list.c ..\windows\pb_sleep_ms.c ..\core\pbauto_heartbeat.c ..\windows\pbauto_heartbeat_init_windows.c

OBJFILES = pbcc_set_state.obj pubnub_pubsubapi.obj pubnub_coreapi.obj pubnub_coreapi_ex.obj pubnub_ccore_pubsub.obj pubnub_ccore.obj pubnub_netcore.obj pbpal_sockets.obj pbpal_resolv_and_connect_sockets.obj pbpal_handle_socket_error.obj pubnub_alloc_std.obj pubnub_assert_std.obj pubnub_generate_uuid.obj pubnub_blocking_io.obj windows_socket_blocking_io.obj pubnub_free_with_timeout_std.obj pbtimespec_elapsed_ms.obj pbbase64.obj pubnub_timers.obj pubnub_json_parse.obj md5.obj pb_strnlen_s.obj pb_strncasecmp.obj pubnub_helper.obj pubnub_version_windows.obj pubnub_generate_uuid_windows.obj pbpal_windows_blocking_io.obj snprintf.obj miniz_tinfl.obj miniz_tdef.obj miniz.obj pbcrc32.obj pbgzip_compress.obj pbgzip_decompress.obj pbcc_subscribe_v2.obj pubnub_subscribe_v2.obj msstopwatch_windows.obj pubnub_url_encode.obj pbcc_advanced_history.obj pubnub_advanced_history.obj pbcc_objects_api.obj pubnub_objects_api.obj pbcc_actions_api.obj pubnub_actions_api.obj pubnub_memory_block.obj pbstr_remove_from_list.obj pb_sleep_ms.obj pbauto_heartbeat.obj pbauto_heartbeat_init_windows.obj

LDLIBS=WindowsApp.lib IPHlpAPI.lib rpcrt4.lib

!ifndef ONLY_PUBSUB_API
ONLY_PUBSUB_API = 0
!endif

!ifndef USE_REVOKE_TOKEN
USE_REVOKE_TOKEN = 0
!endif

!ifndef USE_GRANT_TOKEN
USE_GRANT_TOKEN = 0
!endif

!ifndef USE_FETCH_HISTORY
USE_FETCH_HISTORY = 1
!endif

!ifndef USE_PROXY
USE_PROXY = 1
!endif

!if $(USE_PROXY)
PROXY_INTF_SOURCEFILES = ..\core\pubnub_proxy.c ..\core\pubnub_proxy_core.c ..\core\pbhttp_digest.c ..\core\pbntlm_core.c ..\core\pbntlm_packer_std.c  
PROXY_INTF_OBJFILES = pubnub_proxy.obj pubnub_proxy_core.obj pbhttp_digest.obj pbntlm_core.obj pbntlm_packer_std.obj  
!endif

!if $(USE_FETCH_HISTORY)
FETCH_HIST_SOURCEFILES = ..\core\pubnub_fetch_history.c ..\core\pbcc_fetch_history.c  
FETCH_HIST_OBJFILES = pubnub_fetch_history.obj pbcc_fetch_history.obj  
!endif

DEFINES=-D PUBNUB_THREADSAFE -D PUBNUB_LOG_LEVEL=PUBNUB_LOG_LEVEL_WARNING -D HAVE_STRERROR_S -D PUBNUB_ONLY_PUBSUB_API=$(ONLY_PUBSUB_API) -D PUBNUB_PROXY_API=$(USE_PROXY) -D PUBNUB_USE_WIN_SSPI=0 -D PUBNUB_USE_GRANT_TOKEN_API=$(USE_GRANT_TOKEN) -D PUBNUB_USE_REVOKE_TOKEN_API=$(USE_REVOKE_TOKEN) -D PUBNUB_USE_FETCH_HISTORY=$(USE_FETCH_HISTORY) 
UDEFINES=-D "_UNICODE" -D "UNICODE" -D "WINAPI_FAMILY=WINAPI_FAMILY_APP" -D "__WRL_NO_DEFAULT_LIB__" -D "_CRT_SECURE_NO_WARNINGS" -D "_WINSOCK_DEPRECATED_NO_WARNINGS" -D "__UWP__" -D "HAVE_STRUCT_TIMESPEC"
CFLAGS = -Yu"pch.h" -Zi -Y- -MP -W3  -Gy -Zc:wchar_t $(UDEFINES) $(DEFINES) -Gm- -O2 -sdl -errorReport:prompt -WX- -Zc:forScope /Gd /Oy- /Oi /MD /FC -FU"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.28.29333\lib\x86\store\references\platform.winmd"
# -Zi enables debugging, remove to get a smaller .exe and no .pdb
# -MP use one compiler process for each input, faster on multi-core (ignored by clang-cl)
# -analyze To run the static analyzer (not compatible w/clang-cl)

INCLUDES=-I .. -I . -I ..\core\c99 

all: pubnub_sync.lib pubnub_callback.lib

SYNC_INTF_SOURCEFILES= ..\core\pubnub_ntf_sync.c ..\core\pubnub_sync_subscribe_loop.c ..\core\srand_from_pubnub_time.c
SYNC_INTF_OBJFILES=pubnub_ntf_sync.obj pubnub_sync_subscribe_loop.obj srand_from_pubnub_time.obj

pubnub_sync.lib : $(SOURCEFILES) $(PROXY_INTF_SOURCEFILES) $(FETCH_HIST_SOURCEFILES) $(SYNC_INTF_SOURCEFILES)
	$(CC) -c $(CFLAGS) $(INCLUDES) $(SOURCEFILES) $(PROXY_INTF_SOURCEFILES) $(FETCH_HIST_SOURCEFILES) $(SYNC_INTF_SOURCEFILES)
	lib $(OBJFILES) $(SYNC_INTF_OBJFILES) $(PROXY_INTF_OBJFILES) $(FETCH_HIST_OBJFILES) -OUT:$@

##
# The socket poller module to use. The `poll` poller doesn't have the
# weird restrictions of `select` poller. OTOH, select() on Windows is
# compatible w/BSD sockets select(), while WSAPoll() has some weird
# differences to poll().  The names are the same until the last `_`,
# then it's `poll` vs `select.
SOCKET_POLLER_C=..\lib\sockets\pbpal_ntf_callback_poller_poll.c
SOCKET_POLLER_OBJ=pbpal_ntf_callback_poller_poll.obj

CALLBACK_INTF_SOURCEFILES=pubnub_ntf_callback_windows.c pubnub_get_native_socket.c ..\core\pubnub_timer_list.c ..\lib\sockets\pbpal_adns_sockets.c ..\lib\pubnub_dns_codec.c ..\core\pubnub_dns_servers.c ..\windows\pubnub_dns_system_servers.c ..\lib\pubnub_parse_ipv4_addr.c ..\lib\pubnub_parse_ipv6_addr.c $(SOCKET_POLLER_C) ..\core\pbpal_ntf_callback_queue.c ..\core\pbpal_ntf_callback_admin.c ..\core\pbpal_ntf_callback_handle_timer_list.c ..\core\pubnub_callback_subscribe_loop.c
CALLBACK_INTF_OBJFILES=pubnub_ntf_callback_windows.obj pubnub_get_native_socket.obj pubnub_timer_list.obj pbpal_adns_sockets.obj pubnub_dns_codec.obj pubnub_dns_servers.obj pubnub_dns_system_servers.obj pubnub_parse_ipv4_addr.obj pubnub_parse_ipv6_addr.obj $(SOCKET_POLLER_OBJ) pbpal_ntf_callback_queue.obj pbpal_ntf_callback_admin.obj pbpal_ntf_callback_handle_timer_list.obj pubnub_callback_subscribe_loop.obj

pubnub_callback.lib : $(SOURCEFILES) $(PROXY_INTF_SOURCEFILES) $(FETCH_HIST_SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES)
	$(CC) -c $(CFLAGS) -DPUBNUB_CALLBACK_API $(INCLUDES) $(SOURCEFILES) $(PROXY_INTF_SOURCEFILES) $(FETCH_HIST_SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES)
	lib $(OBJFILES) $(CALLBACK_INTF_OBJFILES) $(PROXY_INTF_OBJFILES) $(FETCH_HIST_OBJFILES) -OUT:$@

clean:
	del *.exe
	del *.obj
	del *.pdb
	del *.il?
	del *.lib
	del *.c~
	del *.h~
	del *~
