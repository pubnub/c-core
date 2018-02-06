SOURCEFILES = ../core/pubnub_pubsubapi.c ../core/pubnub_coreapi.c ../core/pubnub_ccore_pubsub.c ../core/pubnub_ccore.c ../core/pubnub_netcore.c  ../lib/sockets/pbpal_sockets.c ../lib/sockets/pbpal_resolv_and_connect_sockets.c ../core/pubnub_alloc_std.c ../core/pubnub_assert_std.c ../core/pubnub_generate_uuid.c ../core/pubnub_blocking_io.c ../lib/base64/pbbase64.c ../core/pubnub_timers.c ../core/pubnub_json_parse.c ../core/pubnub_proxy.c ../core/pubnub_proxy_core.c ../core/pbhttp_digest.c ../lib/md5/md5.c ../core/pbntlm_core.c ../core/pbntlm_packer_sspi.c pubnub_set_proxy_from_system_windows.c ../core/pubnub_helper.c pubnub_version_windows.c  pubnub_generate_uuid_windows.c pbpal_windows_blocking_io.c

OBJFILES = pubnub_pubsubapi.o pubnub_coreapi.o pubnub_ccore_pubsub.o pubnub_ccore.o pubnub_netcore.o pbpal_sockets.o pbpal_resolv_and_connect_sockets.o pubnub_alloc_std.o pubnub_assert_std.o pubnub_generate_uuid.o pubnub_blocking_io.o pbbase64.o pubnub_timers.o pubnub_json_parse.o pubnub_proxy.o pubnub_proxy_core.o pbhttp_digest.o md5.o pbntlm_core.o pbntlm_packer_sspi.o pubnub_set_proxy_from_system_windows.o pubnub_helper.o pubnub_version_windows.o pubnub_generate_uuid_windows.o pbpal_windows_blocking_io.o


#  -D HAVE_STRERROR_R
CFLAGS = -std=c11 -g  -Wall -D PUBNUB_THREADSAFE -D PUBNUB_LOG_LEVEL=PUBNUB_LOG_LEVEL_DEBUG
# -g enables debugging, remove to get a smaller executable
# -std=c11 enables `%z` and `%ll` in printf() - in general, we're "low" on C11 features
# -fsanitize=address Uses the Address Sanitizer (supported by GCC and Clang)

INCLUDES=-I ../core -I . -I fntest -I ../core/fntest -I ../lib/base64  -I ../lib/md5

LIBFILES=-lws2_32 -lrpcrt4 -lsecur32

##
# We don't build the callback examples, because some older versions of MINGW
# don't have WSAPOLLFD defined. If you have a newer version of MING, uncomment
# the callback examples.
#

all: pubnub_sync_sample.exe cancel_subscribe_sync_sample.exe
# pubnub_console_sync.exe
#subscribe_publish_callback_sample.exe pubnub_callback_sample.exe pubnub_fntest.exe pubnub_console_callback.exe

pubnub_sync.a : $(SOURCEFILES) ../core/pubnub_ntf_sync.c
	"$(CC)" -c $(CFLAGS) $(INCLUDES) $(SOURCEFILES) ../core/pubnub_ntf_sync.c
	$(AR) rcs $@ $(OBJFILES) pubnub_ntf_sync.o 

pubnub_callback.a : $(SOURCEFILES) pubnub_ntf_callback_windows.c pubnub_get_native_socket.c ../core/pubnub_timer_list.c ../lib/sockets/pbpal_adns_sockets.c
	"$(CC)" -c $(CFLAGS) -DPUBNUB_CALLBACK_API $(INCLUDES) $(SOURCEFILES) pubnub_ntf_callback_windows.c pubnub_get_native_socket.c ../core/pubnub_timer_list.c ../lib/sockets/pbpal_adns_sockets.c
	$(AR) rcs $@ $(OBJFILES) pubnub_ntf_callback_windows.o pubnub_get_native_socket.o pubnub_timer_list.o pbpal_adns_sockets.o 

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
