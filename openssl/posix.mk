SOURCEFILES = ../core/pbcc_set_state.c ../core/pubnub_ssl.c ../core/pubnub_pubsubapi.c ../core/pubnub_coreapi.c ../core/pubnub_ccore_pubsub.c ../core/pubnub_ccore.c ../core/pubnub_netcore.c ../lib/sockets/pbpal_resolv_and_connect_sockets.c ../lib/sockets/pbpal_handle_socket_error.c pbpal_openssl.c pbpal_connect_openssl.c pbpal_add_system_certs_posix.c ../core/pubnub_alloc_std.c ../core/pubnub_assert_std.c ../core/pubnub_generate_uuid.c ../core/pubnub_blocking_io.c ../posix/posix_socket_blocking_io.c ../core/pubnub_timers.c ../core/pubnub_json_parse.c  ../core/pubnub_helper.c ../posix/pubnub_version_posix.c ../posix/pubnub_generate_uuid_posix.c pbpal_openssl_blocking_io.c ../lib/base64/pbbase64.c ../lib/pb_strnlen_s.c ../lib/pb_strncasecmp.c ../core/pubnub_coreapi_ex.c ../core/pubnub_free_with_timeout_std.c ../posix/msstopwatch_monotonic_clock.c ../posix/pbtimespec_elapsed_ms.c ../core/pubnub_url_encode.c ../core/pubnub_memory_block.c ../posix/pb_sleep_ms.c

OBJFILES = pbcc_set_state.o pubnub_ssl.o pubnub_pubsubapi.o pubnub_coreapi.o pubnub_ccore_pubsub.o pubnub_ccore.o pubnub_netcore.o pbpal_resolv_and_connect_sockets.o pbpal_handle_socket_error.o pbpal_openssl.o pbpal_connect_openssl.o pbpal_add_system_certs_posix.o pubnub_alloc_std.o pubnub_assert_std.o pubnub_generate_uuid.o pubnub_blocking_io.o posix_socket_blocking_io.o pubnub_timers.o pubnub_json_parse.o pubnub_helper.o pubnub_version_posix.o pubnub_generate_uuid_posix.o pbpal_openssl_blocking_io.o pbbase64.o pb_strnlen_s.o pb_strncasecmp.o pubnub_coreapi_ex.o pubnub_free_with_timeout_std.o msstopwatch_monotonic_clock.o pbtimespec_elapsed_ms.o pubnub_url_encode.o pubnub_memory_block.o pb_sleep_ms.o

ifndef ONLY_PUBSUB_API
ONLY_PUBSUB_API = 0
endif

ifndef USE_PROXY
USE_PROXY = 1
endif

ifndef USE_RETRY_CONFIGURATION
USE_RETRY_CONFIGURATION = 0
endif

ifndef USE_GZIP_COMPRESSION
USE_GZIP_COMPRESSION = 1
endif

ifndef RECEIVE_GZIP_RESPONSE
RECEIVE_GZIP_RESPONSE = 1
endif

ifndef USE_SUBSCRIBE_V2
USE_SUBSCRIBE_V2 = 1
endif

ifndef USE_ADVANCED_HISTORY
USE_ADVANCED_HISTORY = 1
endif

ifndef USE_OBJECTS_API
USE_OBJECTS_API = 1
endif

ifndef USE_ACTIONS_API
USE_ACTIONS_API = 1
endif

ifndef USE_AUTO_HEARTBEAT
USE_AUTO_HEARTBEAT = 1
endif

ifndef USE_REVOKE_TOKEN
USE_REVOKE_TOKEN = 1
endif

ifndef USE_GRANT_TOKEN
USE_GRANT_TOKEN = 1
endif

ifndef USE_FETCH_HISTORY
USE_FETCH_HISTORY = 1
endif

ifndef USE_CRYPTO_API
USE_CRYPTO_API = 1
endif

ifeq ($(USE_PROXY), 1)
SOURCEFILES += ../core/pubnub_proxy.c ../core/pubnub_proxy_core.c ../core/pbhttp_digest.c ../core/pbntlm_core.c ../core/pbntlm_packer_std.c
OBJFILES += pubnub_proxy.o pubnub_proxy_core.o pbhttp_digest.o pbntlm_core.o pbntlm_packer_std.o
endif

ifeq ($(USE_GZIP_COMPRESSION), 1)
SOURCEFILES += ../lib/miniz/miniz_tdef.c ../lib/miniz/miniz.c ../lib/pbcrc32.c ../core/pbgzip_compress.c
OBJFILES += miniz_tdef.o miniz.o pbcrc32.o pbgzip_compress.o
endif

ifeq ($(RECEIVE_GZIP_RESPONSE), 1)
SOURCEFILES += ../lib/miniz/miniz_tinfl.c ../core/pbgzip_decompress.c
OBJFILES += miniz_tinfl.o pbgzip_decompress.o
endif

ifeq ($(USE_SUBSCRIBE_V2), 1)
SOURCEFILES += ../core/pbcc_subscribe_v2.c ../core/pubnub_subscribe_v2.c 
OBJFILES += pbcc_subscribe_v2.o pubnub_subscribe_v2.o 
endif

ifeq ($(USE_ADVANCED_HISTORY), 1)
SOURCEFILES += ../core/pbcc_advanced_history.c ../core/pubnub_advanced_history.c
OBJFILES += pbcc_advanced_history.o pubnub_advanced_history.o
endif

ifeq ($(USE_OBJECTS_API), 1)
SOURCEFILES += ../core/pbcc_objects_api.c ../core/pubnub_objects_api.c
OBJFILES += pbcc_objects_api.o pubnub_objects_api.o
endif

ifeq ($(USE_ACTIONS_API), 1)
SOURCEFILES += ../core/pbcc_actions_api.c ../core/pubnub_actions_api.c
OBJFILES += pbcc_actions_api.o pubnub_actions_api.o
endif

ifeq ($(USE_AUTO_HEARTBEAT), 1)
SOURCEFILES += ../core/pbauto_heartbeat.c ../openssl/pbauto_heartbeat_init_posix.c ../lib/pbstr_remove_from_list.c
OBJFILES += pbauto_heartbeat.o pbauto_heartbeat_init_posix.o pbstr_remove_from_list.o
endif

ifeq ($(USE_GRANT_TOKEN), 1)
GRANT_TOKEN_SOURCEFILES = ../core/pbcc_grant_token_api.c ../core/pubnub_grant_token_api.c ../lib/cbor/cborparser.c ../lib/cbor/cborerrorstrings.c ../lib/cbor/cborparser_dup_string.c
GRANT_TOKEN_OBJFILES = pbcc_grant_token_api.o pubnub_grant_token_api.o cborparser.o cborerrorstrings.o cborparser_dup_string.o
endif

ifeq ($(USE_FETCH_HISTORY), 1)
SOURCEFILES += ../core/pubnub_fetch_history.c ../core/pbcc_fetch_history.c  
OBJFILES += pubnub_fetch_history.o pbcc_fetch_history.o  
endif

ifeq ($(USE_REVOKE_TOKEN), 1)
REVOKE_TOKEN_SOURCEFILES = ../core/pubnub_revoke_token_api.c ../core/pbcc_revoke_token_api.c
REVOKE_TOKEN_OBJFILES = pubnub_revoke_token_api.o pbcc_revoke_token_api.o
endif

ifeq ($(USE_CRYPTO_API), 1)
SOURCEFILES += ../core/pbcc_crypto.c ../core/pbcc_crypto_aes_cbc.c ../core/pbcc_crypto_legacy.c ../core/pubnub_crypto.c ../openssl/pbaes256.c
OBJFILES += pbcc_crypto.o pbcc_crypto_aes_cbc.o pbcc_crypto_legacy.o pubnub_crypto.o pbaes256.o
endif

CFLAGS = -g -D PUBNUB_THREADSAFE -D PUBNUB_LOG_LEVEL=PUBNUB_LOG_LEVEL_WARNING -Wall -D PUBNUB_ONLY_PUBSUB_API=$(ONLY_PUBSUB_API) -D PUBNUB_PROXY_API=$(USE_PROXY) -D PUBNUB_USE_RETRY_CONFIGURATION=$(USE_RETRY_CONFIGURATION) -D PUBNUB_USE_GZIP_COMPRESSION=$(USE_GZIP_COMPRESSION) -D PUBNUB_RECEIVE_GZIP_RESPONSE=$(RECEIVE_GZIP_RESPONSE) -D PUBNUB_USE_SUBSCRIBE_V2=$(USE_SUBSCRIBE_V2) -D PUBNUB_USE_OBJECTS_API=$(USE_OBJECTS_API) -D PUBNUB_USE_ACTIONS_API=$(USE_ACTIONS_API) -D PUBNUB_USE_AUTO_HEARTBEAT=$(USE_AUTO_HEARTBEAT) -D PUBNUB_USE_GRANT_TOKEN_API=$(USE_GRANT_TOKEN) -D PUBNUB_USE_REVOKE_TOKEN_API=$(USE_REVOKE_TOKEN) -D PUBNUB_USE_FETCH_HISTORY=$(USE_FETCH_HISTORY) -D PUBNUB_CRYPTO_API=$(USE_CRYPTO_API)
# -g enables debugging, remove to get a smaller executable
# -fsanitize=address Use AddressSanitizer
# -fsanitize=thread Use ThreadSanitizer

OS := $(shell uname)
ifeq ($(OS),Darwin)
SOURCEFILES += ../posix/monotonic_clock_get_time_darwin.c
OBJFILES += monotonic_clock_get_time_darwin.o
LDLIBS = -lpthread -lssl -lcrypto -L/usr/local/opt/openssl/lib
CFLAGS += -I/usr/local/opt/openssl/include
else
SOURCEFILES += ../posix/monotonic_clock_get_time_posix.c
OBJFILES += monotonic_clock_get_time_posix.o
LDLIBS=-lrt -lpthread -lssl -lcrypto
endif


INCLUDES=-I .. -I . -I ../lib/base64/


all: pubnub_sync_sample pubnub_sync_grant_token_sample pubnub_sync_revoke_token_sample pubnub_objects_secretkey_sample metadata cancel_subscribe_sync_sample pubnub_sync_subloop_sample pubnub_publish_via_post_sample pubnub_publish_via_post_secretkey_sample pubnub_advanced_history_sample pubnub_fetch_history_sample pubnub_callback_sample subscribe_publish_callback_sample pubnub_callback_subloop_sample pubnub_fntest pubnub_console_sync pubnub_console_callback pubnub_crypto_sync_sample subscribe_publish_from_callback publish_callback_subloop_sample publish_queue_callback_subloop pubnub_crypto_module_sample

SYNC_INTF_SOURCEFILES=../core/pubnub_ntf_sync.c ../core/pubnub_sync_subscribe_loop.c ../core/srand_from_pubnub_time.c
SYNC_INTF_OBJFILES=pubnub_ntf_sync.o pubnub_sync_subscribe_loop.o srand_from_pubnub_time.o

pubnub_sync.a : $(SOURCEFILES) $(SYNC_INTF_SOURCEFILES) $(REVOKE_TOKEN_SOURCEFILES) $(GRANT_TOKEN_SOURCEFILES)
	$(CC) -c $(CFLAGS) -D PUBNUB_RAND_INIT_VECTOR=0 $(INCLUDES) $(SOURCEFILES) $(SYNC_INTF_SOURCEFILES) $(REVOKE_TOKEN_SOURCEFILES) $(GRANT_TOKEN_SOURCEFILES)
	ar rcs pubnub_sync.a $(OBJFILES) $(SYNC_INTF_OBJFILES) $(REVOKE_TOKEN_OBJFILES) $(GRANT_TOKEN_OBJFILES)

pubnub_sync_dynamiciv.a : $(SOURCEFILES) $(SYNC_INTF_SOURCEFILES) $(REVOKE_TOKEN_SOURCEFILES) $(GRANT_TOKEN_SOURCEFILES)
	$(CC) -c $(CFLAGS) -D PUBNUB_RAND_INIT_VECTOR=1 $(INCLUDES) $(SOURCEFILES) $(SYNC_INTF_SOURCEFILES) $(REVOKE_TOKEN_SOURCEFILES) $(GRANT_TOKEN_SOURCEFILES)
	ar rcs pubnub_sync_dynamiciv.a $(OBJFILES) $(SYNC_INTF_OBJFILES) $(REVOKE_TOKEN_OBJFILES) $(GRANT_TOKEN_OBJFILES)

CALLBACK_INTF_SOURCEFILES=pubnub_ntf_callback_posix.c pubnub_get_native_socket.c ../core/pubnub_timer_list.c ../lib/sockets/pbpal_ntf_callback_poller_poll.c ../lib/sockets/pbpal_adns_sockets.c ../lib/pubnub_dns_codec.c ../core/pbpal_ntf_callback_queue.c ../core/pbpal_ntf_callback_admin.c ../core/pbpal_ntf_callback_handle_timer_list.c  ../core/pubnub_callback_subscribe_loop.c
CALLBACK_INTF_OBJFILES=pubnub_ntf_callback_posix.o pubnub_get_native_socket.o pubnub_timer_list.o pbpal_ntf_callback_poller_poll.o pbpal_adns_sockets.o pubnub_dns_codec.o pbpal_ntf_callback_queue.o pbpal_ntf_callback_admin.o pbpal_ntf_callback_handle_timer_list.o pubnub_callback_subscribe_loop.o

ifndef USE_DNS_SERVERS
USE_DNS_SERVERS = 1
endif

ifndef USE_IPV6
USE_IPV6 = 1
endif

ifeq ($(USE_DNS_SERVERS), 1)
CALLBACK_INTF_SOURCEFILES += ../core/pubnub_dns_servers.c ../posix/pubnub_dns_system_servers.c ../lib/pubnub_parse_ipv4_addr.c
CALLBACK_INTF_OBJFILES += pubnub_dns_servers.o pubnub_dns_system_servers.o pubnub_parse_ipv4_addr.o
else ifeq ($(USE_PROXY), 1)
CALLBACK_INTF_SOURCEFILES += ../lib/pubnub_parse_ipv4_addr.c
CALLBACK_INTF_OBJFILES += pubnub_parse_ipv4_addr.o
endif

ifeq ($(USE_IPV6), 1)
CALLBACK_INTF_SOURCEFILES += ../lib/pubnub_parse_ipv6_addr.c
CALLBACK_INTF_OBJFILES += pubnub_parse_ipv6_addr.o
endif

CFLAGS_CALLBACK = -D PUBNUB_USE_IPV6=$(USE_IPV6) -D PUBNUB_SET_DNS_SERVERS=$(USE_DNS_SERVERS)

pubnub_callback.a : $(SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES) $(REVOKE_TOKEN_SOURCEFILES) $(GRANT_TOKEN_SOURCEFILES) 
	$(CC) -c $(CFLAGS) $(CFLAGS_CALLBACK) $(INCLUDES) -D PUBNUB_CALLBACK_API $(SOURCEFILES) $(CALLBACK_INTF_SOURCEFILES) $(REVOKE_TOKEN_SOURCEFILES) $(GRANT_TOKEN_SOURCEFILES) 
	ar rcs pubnub_callback.a $(OBJFILES) $(CALLBACK_INTF_OBJFILES) $(REVOKE_TOKEN_OBJFILES) $(GRANT_TOKEN_OBJFILES) 

pubnub_sync_sample: ../core/samples/pubnub_sync_sample.c pubnub_sync.a
	$(CC) -o $@ $(CFLAGS) $(INCLUDES) ../core/samples/pubnub_sync_sample.c pubnub_sync.a $(LDLIBS)

pubnub_sync_grant_token_sample: ../core/samples/pubnub_sync_grant_token_sample.c pubnub_sync_dynamiciv.a
	$(CC) -o $@ $(CFLAGS) $(INCLUDES) ../core/samples/pubnub_sync_grant_token_sample.c pubnub_sync_dynamiciv.a $(LDLIBS)

pubnub_sync_revoke_token_sample: ../core/samples/pubnub_sync_revoke_token_sample.c pubnub_sync_dynamiciv.a
	$(CC) -o $@ $(CFLAGS) $(INCLUDES) ../core/samples/pubnub_sync_revoke_token_sample.c pubnub_sync_dynamiciv.a $(LDLIBS)

pubnub_encrypt_decrypt_static_iv_sample: ../core/samples/pubnub_encrypt_decrypt_iv_sample.c pubnub_sync.a
	$(CC) $(CFLAGS) $(INCLUDES) -o pubnub_encrypt_decrypt_static_iv_sample ../core/samples/pubnub_encrypt_decrypt_iv_sample.c pubnub_sync.a $(LDLIBS)

pubnub_encrypt_decrypt_dynamic_iv_sample: ../core/samples/pubnub_encrypt_decrypt_iv_sample.c pubnub_sync_dynamiciv.a
	$(CC) $(CFLAGS) $(INCLUDES) -o pubnub_encrypt_decrypt_dynamic_iv_sample ../core/samples/pubnub_encrypt_decrypt_iv_sample.c pubnub_sync_dynamiciv.a $(LDLIBS)

pubnub_objects_secretkey_sample: ../core/samples/pubnub_objects_secretkey_sample.c pubnub_sync.a	
	$(CC) -o $@ $(CFLAGS) $(INCLUDES) ../core/samples/pubnub_objects_secretkey_sample.c pubnub_sync.a $(LDLIBS)

metadata: ../core/samples/metadata.c pubnub_sync.a
	$(CC) -o $@ $(CFLAGS) $(INCLUDES) ../core/samples/metadata.c pubnub_sync.a $(LDLIBS)

pubnub_crypto_sync_sample: ../core/samples/pubnub_crypto_sync_sample.c pubnub_sync.a
	$(CC) -o $@ $(CFLAGS) $(INCLUDES) ../core/samples/pubnub_crypto_sync_sample.c pubnub_sync.a $(LDLIBS)

pubnub_encrypt_decrypt_sample: ../core/samples/pubnub_encrypt_decrypt_sample.c pubnub_sync.a
	$(CC) -o $@ $(CFLAGS) $(INCLUDES) ../core/samples/pubnub_encrypt_decrypt_sample.c pubnub_sync.a $(LDLIBS)

cancel_subscribe_sync_sample: ../core/samples/cancel_subscribe_sync_sample.c pubnub_sync.a
	$(CC) -o $@ $(CFLAGS) $(INCLUDES) ../core/samples/cancel_subscribe_sync_sample.c pubnub_sync.a $(LDLIBS)

pubnub_sync_subloop_sample: ../core/samples/pubnub_sync_subloop_sample.c pubnub_sync.a
	$(CC) -o $@ $(CFLAGS) $(INCLUDES) ../core/samples/pubnub_sync_subloop_sample.c pubnub_sync.a $(LDLIBS)

pubnub_publish_via_post_sample: ../core/samples/pubnub_publish_via_post_sample.c pubnub_sync.a
	$(CC) -o $@ $(CFLAGS) $(INCLUDES) ../core/samples/pubnub_publish_via_post_sample.c pubnub_sync.a $(LDLIBS)

pubnub_publish_via_post_secretkey_sample: ../core/samples/pubnub_publish_via_post_secretkey_sample.c pubnub_sync.a
	$(CC) -o $@ $(CFLAGS) $(INCLUDES) ../core/samples/pubnub_publish_via_post_secretkey_sample.c pubnub_sync.a $(LDLIBS)

pubnub_advanced_history_sample: ../core/samples/pubnub_advanced_history_sample.c pubnub_sync.a
	$(CC) -o $@ $(CFLAGS) $(INCLUDES) ../core/samples/pubnub_advanced_history_sample.c pubnub_sync.a $(LDLIBS)

pubnub_fetch_history_sample: ../core/samples/pubnub_fetch_history_sample.c pubnub_sync.a
	$(CC) -o $@ $(CFLAGS) $(INCLUDES) ../core/samples/pubnub_fetch_history_sample.c pubnub_sync.a $(LDLIBS)

pubnub_callback_sample: ../core/samples/pubnub_callback_sample.c pubnub_callback.a
	$(CC) -o $@ -D PUBNUB_CALLBACK_API $(CFLAGS) $(CFLAGS_CALLBACK) $(INCLUDES) ../core/samples/pubnub_callback_sample.c pubnub_callback.a $(LDLIBS)

subscribe_publish_callback_sample: ../core/samples/subscribe_publish_callback_sample.c pubnub_callback.a
	$(CC) -o $@ -D PUBNUB_CALLBACK_API $(CFLAGS) $(CFLAGS_CALLBACK) $(INCLUDES) ../core/samples/subscribe_publish_callback_sample.c pubnub_callback.a $(LDLIBS)

pubnub_callback_subloop_sample: ../core/samples/pubnub_callback_subloop_sample.c pubnub_callback.a
	$(CC) -o $@ -D PUBNUB_CALLBACK_API $(CFLAGS) $(CFLAGS_CALLBACK) $(INCLUDES) ../core/samples/pubnub_callback_subloop_sample.c pubnub_callback.a $(LDLIBS)

subscribe_publish_from_callback: ../core/samples/subscribe_publish_from_callback.c pubnub_callback.a
	$(CC) -o $@ -D PUBNUB_CALLBACK_API $(CFLAGS) $(CFLAGS_CALLBACK) $(INCLUDES) ../core/samples/subscribe_publish_from_callback.c pubnub_callback.a $(LDLIBS)

publish_callback_subloop_sample: ../core/samples/publish_callback_subloop_sample.c pubnub_callback.a
	$(CC) -o $@ -D PUBNUB_CALLBACK_API $(CFLAGS) $(CFLAGS_CALLBACK) $(INCLUDES) ../core/samples/publish_callback_subloop_sample.c pubnub_callback.a $(LDLIBS)

publish_queue_callback_subloop: ../core/samples/publish_queue_callback_subloop.c pubnub_callback.a
	$(CC) -o $@ -D PUBNUB_CALLBACK_API $(CFLAGS) $(CFLAGS_CALLBACK) $(INCLUDES) ../core/samples/publish_queue_callback_subloop.c pubnub_callback.a $(LDLIBS)

pubnub_crypto_module_sample: ../core/samples/pubnub_crypto_module_sample.c pubnub_sync.a
	$(CC) -o $@ $(CFLAGS) $(INCLUDES) -I ../core/ ../core/samples/pubnub_crypto_module_sample.c pubnub_sync.a $(LDLIBS)


pubnub_fntest: ../core/fntest/pubnub_fntest.c ../core/fntest/pubnub_fntest_basic.c ../core/fntest/pubnub_fntest_medium.c ../posix/fntest/pubnub_fntest_posix.c ../posix/fntest/pubnub_fntest_runner.c pubnub_sync.a
	$(CC) -o $@ $(CFLAGS) $(INCLUDES) ../core/fntest/pubnub_fntest.c ../core/fntest/pubnub_fntest_basic.c ../core/fntest/pubnub_fntest_medium.c  ../posix/fntest/pubnub_fntest_posix.c ../posix/fntest/pubnub_fntest_runner.c pubnub_sync.a $(LDLIBS) -lpthread

CONSOLE_SOURCEFILES=../core/samples/console/pubnub_console.c ../core/samples/console/pnc_helpers.c ../core/samples/console/pnc_readers.c ../core/samples/console/pnc_subscriptions.c

pubnub_console_sync: $(CONSOLE_SOURCEFILES) ../core/samples/console/pnc_ops_sync.c pubnub_sync.a
	$(CC) -o $@ $(CFLAGS) $(INCLUDES) $(CONSOLE_SOURCEFILES) ../core/samples/console/pnc_ops_sync.c  pubnub_sync.a $(LDLIBS)

pubnub_console_callback: $(CONSOLE_SOURCEFILES) ../core/samples/console/pnc_ops_callback.c pubnub_callback.a
	$(CC) -o $@ -D PUBNUB_CALLBACK_API $(CFLAGS) $(CFLAGS_CALLBACK) $(INCLUDES) $(CONSOLE_SOURCEFILES) ../core/samples/console/pnc_ops_callback.c pubnub_callback.a $(LDLIBS)


clean:
	find . -type d -iname "*.dSYM" -exec rm -rf {} \+
	find . -type f ! -name "*.*" -o -name "*.a" -o -name "*.o" | xargs -r rm -rf -rf
