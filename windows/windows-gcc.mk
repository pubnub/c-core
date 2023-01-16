SOURCEFILES = ../core/pbcc_set_state.c ../core/pubnub_pubsubapi.c ../core/pubnub_coreapi.c ../core/pubnub_coreapi_ex.c ../core/pubnub_ccore_pubsub.c ../core/pubnub_ccore.c ../core/pubnub_netcore.c ../lib/sockets/pbpal_sockets.c ../lib/sockets/pbpal_resolv_and_connect_sockets.c ../core/pubnub_alloc_std.c ../core/pubnub_assert_std.c ../core/pubnub_generate_uuid.c ../core/pubnub_blocking_io.c ../windows/windows_socket_blocking_io.c ../core/pubnub_free_with_timeout_std.c ../lib/base64/pbbase64.c ../core/pubnub_timers.c ../core/pubnub_json_parse.c ../lib/md5/md5.c ../core/pubnub_helper.c pubnub_version_windows.c  pubnub_generate_uuid_windows.c pbpal_windows_blocking_io.c ../core/c99/snprintf.c ../lib/miniz/miniz_tinfl.c ../lib/miniz/miniz_tdef.c ../lib/miniz/miniz.c ../lib/pbcrc32.c ../core/pbgzip_compress.c ../core/pbgzip_decompress.c ../core/pubnub_subscribe_v2.c msstopwatch_windows.c ../core/pubnub_url_encode.c ../core/pbcc_advanced_history.c ../core/pubnub_advanced_history.c

OBJFILES = pbcc_set_state.obj pubnub_pubsubapi.obj pubnub_coreapi.obj pubnub_coreapi_ex.obj pubnub_ccore_pubsub.obj pubnub_ccore.obj pubnub_netcore.obj pbpal_sockets.obj pbpal_resolv_and_connect_sockets.obj pubnub_alloc_std.obj pubnub_assert_std.obj pubnub_generate_uuid.obj pubnub_blocking_io.obj windows_socket_blocking_io.obj pubnub_free_with_timeout_std.obj pbbase64.obj pubnub_timers.obj pubnub_json_parse.obj md5.obj pubnub_helper.obj pubnub_version_windows.obj pubnub_generate_uuid_windows.obj pbpal_windows_blocking_io.obj snprintf.obj miniz_tinfl.obj miniz_tdef.obj miniz.obj pbcrc32.obj pbgzip_compress.obj pbgzip_decompress.obj pubnub_subscribe_v2.obj msstopwatch_windows.obj pubnub_url_encode.obj pbcc_advanced_history.obj pubnub_advanced_history.obj


!ifndef ONLY_PUBSUB_API
ONLY_PUBSUB_API = 0
!endif

!ifndef USE_PROXY
USE_PROXY = 1
!endif

!if $(USE_PROXY)
PROXY_INTF_SOURCEFILES = ..\core\pubnub_proxy.c ..\core\pubnub_proxy_core.c ..\core\pbhttp_digest.c ..\core\pbntlm_core.c ..\core\pbntlm_packer_sspi.c pubnub_set_proxy_from_system_windows.c 
PROXY_INTF_OBJFILES = pubnub_proxy.obj pubnub_proxy_core.obj pbhttp_digest.obj pbntlm_core.obj pbntlm_packer_sspi.obj pubnub_set_proxy_from_system_windows.obj 
!endif

DEFINES=-D PUBNUB_THREADSAFE -D PUBNUB_LOG_LEVEL=PUBNUB_LOG_LEVEL_WARNING -D HAVE_STRERROR_S -D PUBNUB_ONLY_PUBSUB_API=$(ONLY_PUBSUB_API) -D PUBNUB_PROXY_API=$(USE_PROXY)


#  -D HAVE_STRERROR_R
CFLAGS = -std=c11 -g  -Wall -D PUBNUB_THREADSAFE -D PUBNUB_LOG_LEVEL=PUBNUB_LOG_LEVEL_DEBUG
# -g enables debugging, remove to get a smaller executable
# -std=c11 enables `%z` and `%ll` in printf() - in general, we're "low" on C11 features
# -fsanitize=address Uses the Address Sanitizer (supported by GCC and Clang)

INCLUDES=-I .. -I .

LIBFILES=-lws2_32 -lrpcrt4 -lsecur32

##
# We don't build the callback examples, because some older versions of MINGW
# don't have WSAPOLLFD defined. If you have a newer version of MING, uncomment
# the callback examples.
#

all: pubnub_sync_sample.exe cancel_subscribe_sync_sample.exe
# pubnub_console_sync.exe
#subscribe_publish_callback_sample.exe pubnub_callback_sample.exe pubnub_fntest.exe pubnub_console_callback.exe

SYNC_INTF_SOURCEFILES= ../core/pubnub_ntf_sync.c ../core/pubnub_sync_subscribe_loop.c ../core/srand_from_pubnub_time.c
SYNC_INTF_OBJFILES=pubnub_ntf_sync.obj pubnub_sync_subscribe_loop.obj srand_from_pubnub_time.obj


pubnub_sync.a: $(SOURCEFILES) $(PROXY_INTF_SOURCEFILES) $(SYNC_INTF_SOURCEFILES)
	"$(CC)" -c $(CFLAGS) $(INCLUDES) $(SOURCEFILES) $(PROXY_INTF_SOURCEFILES) $(SYNC_INTF_SOURCEFILES)
	$(AR) rcs $@ $(OBJFILES) $(SYNC_INTF_OBJFILES) $(PROXY_INTF_OBJFILES)

##
# The socket poller module to use. The `poll` poller doesn't have the
# weird restrictions of `select` poller. OTOH, select() on Windows is
# compatible w/BSD sockets select(), while WSAPoll() has some weird
# differences to poll().  The names are the same until the last `_`,
# then it's `poll` vs `select.
SOCKET_POLLER_C=..\lib\sockets\pbpal_ntf_callback_poller_poll.c
SOCKET_POLLER_OBJ=pbpal_ntf_callback_poller_poll.obj

CALLBACK_INTF_SOURCEFILES=pubnub_ntf_callback_windows.c pubnub_get_native_socket.c ../core/pubnub_timer_list.c ../lib/sockets/pbpal_adns_sockets.c ../lib/pubnub_dns_codec.c ../core/pubnub_dns_servers.c ../windows/pubnub_dns_system_servers.c ../lib/pubnub_parse_ipv4_addr.c ../lib/pubnub_parse_ipv6_addr.c $(SOCKET_POLLER_C) ../core/pbpal_ntf_callback_queue.c ../core/pbpal_ntf_callback_admin.c ../core/pbpal_ntf_callback_handle_timer_list.c ../core/pubnub_callback_subscribe_loop.c
CALLBACK_INTF_OBJFILES=pubnub_ntf_callback_windows.obj pubnub_get_native_socket.obj pubnub_timer_list.obj pbpal_adns_sockets.obj pubnub_dns_codec.obj pubnub_dns_servers.obj pubnub_dns_system_servers.obj pubnub_parse_ipv4_addr.obj pubnub_parse_ipv6_addr.obj $(SOCKET_POLLER_OBJ) pbpal_ntf_callback_queue.obj pbpal_ntf_callback_admin.obj pbpal_ntf_callback_handle_timer_list.obj pubnub_callback_subscribe_loop.obj


pubnub_callback.a : $(SOURCEFILES) $(PROXY_INTF_SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES)
	"$(CC)" -c $(CFLAGS) -DPUBNUB_CALLBACK_API $(INCLUDES) $(SOURCEFILES) $(PROXY_INTF_SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES)
	$(AR) rcs $@ $(OBJFILES) $(PROXY_INTF_SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES)

pubnub_sync_sample.exe: ../core/samples/pubnub_sync_sample.c pubnub_sync.a
	"$(CC)" -o $@ $(CFLAGS) $(INCLUDES) ../core/samples/pubnub_sync_sample.c pubnub_sync.a $(LIBFILES)

cancel_subscribe_sync_sample.exe: ../core/samples/cancel_subscribe_sync_sample.c pubnub_sync.a
	"$(CC)" -o $@ $(CFLAGS) $(INCLUDES) ../core/samples/cancel_subscribe_sync_sample.c pubnub_sync.a $(LIBFILES)

#pubnub_callback_sample.exe: ../core/samples/pubnub_callback_sample.c $(SOURCEFILES) pubnub_ntf_callback_windows.c
#	$(CC) -o $@ $(CFLAGS) -DPUBNUB_CALLBACK_API ../core/samples/pubnub_callback_sample.c  $(SOURCEFILES) pubnub_ntf_callback_windows.c $(LIBFILES)

#subscribe_publish_callback_sample.exe: ../core/samples/subscribe_publish_callback_sample.c $(SOURCEFILES) pubnub_ntf_callback_windows.c
#	$(CC) -o $@ $(CFLAGS) -DPUBNUB_CALLBACK_API ../core/samples/subscribe_publish_callback_sample.c  $(SOURCEFILES) pubnub_ntf_callback_windows.c $(LIBFILES)

pubnub_fntest.exe: ../core/fntest/pubnub_fntest.c ../core/fntest/pubnub_fntest_basic.c ../core/fntest/pubnub_fntest_medium.c  fntest/pubnub_fntest_windows.c fntest/pubnub_fntest_runner.c $(SOURCEFILES)  ../core/pubnub_ntf_sync.c
	$(CC) -o $@ $(CFLAGS) ../core/fntest/pubnub_fntest.c ../core/fntest/pubnub_fntest_basic.c ../core/fntest/pubnub_fntest_medium.c fntest/pubnub_fntest_windows.c fntest/pubnub_fntest_runner.c $(SOURCEFILES)  ../core/pubnub_ntf_sync.c $(LIBFILES)

clean:
	$(RM) *.exe
	$(RM) *.o
	$(RM) *.a
