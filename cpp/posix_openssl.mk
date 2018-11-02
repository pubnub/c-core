SOURCEFILES = ../core/pubnub_pubsubapi.c ../core/pubnub_coreapi.c ../core/pubnub_ccore_pubsub.c ../core/pubnub_ccore.c ../core/pubnub_netcore.c ../lib/sockets/pbpal_resolv_and_connect_sockets.c ../openssl/pbpal_openssl.c ../openssl/pbpal_connect_openssl.c  ../openssl/pbpal_add_system_certs_posix.c ../core/pubnub_alloc_std.c ../core/pubnub_assert_std.c ../core/pubnub_generate_uuid.c ../core/pubnub_blocking_io.c ../posix/posix_socket_blocking_io.c ../core/pubnub_free_with_timeout_std.c ../core/pubnub_timers.c ../core/pubnub_json_parse.c ../lib/md5/md5.c ../lib/base64/pbbase64.c ../core/pubnub_helper.c  ../openssl/pubnub_version_openssl.c ../posix/pubnub_generate_uuid_posix.c ../openssl/pbpal_openssl_blocking_io.c ../core/pubnub_crypto.c ../core/pubnub_coreapi_ex.c ../openssl/pbaes256.c  ../core/pubnub_subscribe_v2.c ../posix/msstopwatch_monotonic_clock.c

ifndef USE_PROXY
USE_PROXY = 1
endif

ifndef USE_DNS_SERVERS
USE_DNS_SERVERS = 1
endif

ifndef RECEIVE_GZIP_RESPONSE
RECEIVE_GZIP_RESPONSE = 1
endif

ifeq ($(USE_PROXY), 1)
SOURCEFILES += ../core/pubnub_proxy.c ../core/pubnub_proxy_core.c ../core/pbhttp_digest.c ../core/pbntlm_core.c ../core/pbntlm_packer_std.c
endif

ifeq ($(USE_DNS_SERVERS), 1)
SOURCEFILES += ../core/pubnub_dns_servers.c ../posix/pubnub_dns_system_servers.c ../lib/pubnub_parse_ipv4_addr.c
OBJFILES += pubnub_dns_servers.o pubnub_dns_system_servers.o pubnub_parse_ipv4_addr.o
endif

ifeq ($(RECEIVE_GZIP_RESPONSE), 1)
SOURCEFILES += ../lib/miniz/miniz_tinfl.c ../core/pbgzip_decompress.c
OBJFILES += miniz_tinfl.o pbgzip_decompress.o
endif

CFLAGS =-g -I .. -I . -I ../openssl -Wall -D PUBNUB_THREADSAFE -D PUBNUB_LOG_LEVEL=PUBNUB_LOG_LEVEL_WARNING
# -g enables debugging, remove to get a smaller executable

OS := $(shell uname)
ifeq ($(OS),Darwin)
SOURCEFILES += ../posix/monotonic_clock_get_time_darwin.c
LDLIBS=-lpthread -lssl -lcrypto -L/usr/local/opt/openssl/lib
CFLAGS += -I/usr/local/opt/openssl/include
else
SOURCEFILES += ../posix/monotonic_clock_get_time_posix.c
LDLIBS=-lrt -lpthread -lssl -lcrypto
endif


all: openssl/pubnub_sync_sample openssl/pubnub_callback_sample openssl/pubnub_callback_cpp11_sample openssl/cancel_subscribe_sync_sample openssl/subscribe_publish_callback_sample openssl/futres_nesting_sync openssl/fntest_runner openssl/futres_nesting_callback openssl/futres_nesting_callback_cpp11


openssl/pubnub_sync_sample: samples/pubnub_sample.cpp $(SOURCEFILES) ../core/pubnub_ntf_sync.c pubnub_futres_sync.cpp
	$(CXX) -o $@ $(CFLAGS) -x c++ samples/pubnub_sample.cpp ../core/pubnub_ntf_sync.c pubnub_futres_sync.cpp $(SOURCEFILES) $(LDLIBS)

openssl/cancel_subscribe_sync_sample: samples/cancel_subscribe_sync_sample.cpp $(SOURCEFILES) ../core/pubnub_ntf_sync.c pubnub_futres_sync.cpp
	$(CXX) -o $@ $(CFLAGS) -x c++ samples/cancel_subscribe_sync_sample.cpp  ../core/pubnub_ntf_sync.c pubnub_futres_sync.cpp $(SOURCEFILES) $(LDLIBS)

openssl/futres_nesting_sync: samples/futres_nesting.cpp $(SOURCEFILES) ../core/pubnub_ntf_sync.c pubnub_futres_sync.cpp
	$(CXX) -o $@ $(CFLAGS) -x c++ samples/futres_nesting.cpp ../core/pubnub_ntf_sync.c pubnub_futres_sync.cpp $(SOURCEFILES) $(LDLIBS)

openssl/fntest_runner: fntest/pubnub_fntest_runner.cpp $(SOURCEFILES)  ../core/pubnub_ntf_sync.c pubnub_futres_sync.cpp fntest/pubnub_fntest.cpp fntest/pubnub_fntest_basic.cpp fntest/pubnub_fntest_medium.cpp
	$(CXX) -o $@ --std=c++11 $(CFLAGS) -x c++ fntest/pubnub_fntest_runner.cpp ../core/pubnub_ntf_sync.c pubnub_futres_sync.cpp fntest/pubnub_fntest.cpp fntest/pubnub_fntest_basic.cpp fntest/pubnub_fntest_medium.cpp $(SOURCEFILES) $(LDLIBS) 

##
# The socket poller module to use. You should use the `poll` poller it
# doesn't have the weird restrictions of `select` poller. OTOH,
# select() on Windows is compatible w/BSD sockets select(), while
# WSAPoll() has some weird differences to poll().  The names are the
# same until the last `_`, then it's `poll` vs `select.
SOCKET_POLLER_C=../lib/sockets/pbpal_ntf_callback_poller_poll.c
SOCKET_POLLER_OBJ=pbpal_ntf_callback_poller_poll.o

CALLBACK_INTF_SOURCEFILES= ../openssl/pubnub_ntf_callback_posix.c ../openssl/pubnub_get_native_socket.c ../core/pubnub_timer_list.c ../lib/sockets/pbpal_adns_sockets.c $(SOCKET_POLLER_C) ../core/pbpal_ntf_callback_queue.c ../core/pbpal_ntf_callback_admin.c ../core/pbpal_ntf_callback_handle_timer_list.c  ../core/pubnub_callback_subscribe_loop.c
CALLBACK_INTF_OBJFILES= pubnub_ntf_callback_posix.o pubnub_get_native_socket.o pubnub_timer_list.o pbpal_adns_sockets.o $(SOCKET_POLLER_OBJ) pbpal_ntf_callback_queue.o pbpal_ntf_callback_admin.o pbpal_ntf_callback_handle_timer_list.o pubnub_callback_subscribe_loop.o

openssl/pubnub_callback_sample: samples/pubnub_sample.cpp $(SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_posix.cpp
	$(CXX) -o $@ -D PUBNUB_CALLBACK_API $(CFLAGS) -x c++ samples/pubnub_sample.cpp $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_posix.cpp $(SOURCEFILES) $(LDLIBS)

openssl/pubnub_callback_cpp11_sample: samples/pubnub_sample.cpp $(SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_cpp11.cpp
	$(CXX) -o $@ --std=c++11 -D PUBNUB_CALLBACK_API $(CFLAGS) -x c++ samples/pubnub_sample.cpp $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_cpp11.cpp $(SOURCEFILES) $(LDLIBS)

openssl/subscribe_publish_callback_sample: samples/subscribe_publish_callback_sample.cpp $(SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_posix.cpp
	$(CXX) -o $@ -D PUBNUB_CALLBACK_API $(CFLAGS) -x c++ samples/subscribe_publish_callback_sample.cpp $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_posix.cpp $(SOURCEFILES) $(LDLIBS)

openssl/futres_nesting_callback: samples/futres_nesting.cpp $(SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_posix.cpp
	$(CXX) -o $@ -D PUBNUB_CALLBACK_API $(CFLAGS) -x c++ samples/futres_nesting.cpp $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_posix.cpp $(SOURCEFILES) $(LDLIBS)

openssl/futres_nesting_callback_cpp11: samples/futres_nesting.cpp $(SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_cpp11.cpp
	$(CXX) -o $@ --std=c++11 -D PUBNUB_CALLBACK_API $(CFLAGS) -x c++ samples/futres_nesting.cpp $(CALLBACK_INTF_SOURCEFILES) pubnub_futres_cpp11.cpp $(SOURCEFILES) $(LDLIBS)

clean:
	rm openssl/pubnub_sync_sample openssl/pubnub_callback_sample openssl/pubnub_callback_cpp11_sample openssl/cancel_subscribe_sync_sample openssl/subscribe_publish_callback_sample openssl/futres_nesting_sync openssl/fntest_runner openssl/futres_nesting_callback openssl/futres_nesting_callback_cpp11
