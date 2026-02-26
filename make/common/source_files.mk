# Common PubNub SDK source files.
# Description: Makefile used to declare scoped set of source files to build
#              PubNub SDK core and its features.


###############################################################################
#                          PubNub Core source files                           #
###############################################################################

# PubNub SDK core source files used for all platforms.
CORE_SOURCE_FILES = \
    ../core/pbcc_set_state.c                          \
    ../core/pubnub_alloc_std.c                        \
    ../core/pubnub_assert_std.c                       \
    ../core/pubnub_blocking_io.c                      \
    ../core/pubnub_ccore.c                            \
    ../core/pubnub_ccore_pubsub.c                     \
    ../core/pubnub_coreapi.c                          \
    ../core/pubnub_coreapi_ex.c                       \
    ../core/pubnub_free_with_timeout_std.c            \
    ../core/pubnub_generate_uuid.c                    \
    ../core/pubnub_generate_uuid_v3_md5.c             \
    ../core/pubnub_helper.c                           \
    ../core/pubnub_json_parse.c                       \
    ../core/pubnub_memory_block.c                     \
    ../core/pubnub_netcore.c                          \
    ../core/pubnub_pubsubapi.c                        \
    ../core/pubnub_timers.c                           \
    ../core/pubnub_url_encode.c                       \
    ../lib/base64/pbbase64.c                          \
    ../lib/pb_strncasecmp.c                           \
    ../lib/pb_strnlen_s.c                             \
    ../lib/sockets/pbpal_handle_socket_error.c        \
    ../lib/sockets/pbpal_resolv_and_connect_sockets.c

# `CORE_SOURCE_FILES` extension for POSIX build.
CORE_SOURCE_FILES_POSIX = \
    ../posix/msstopwatch_monotonic_clock.c            \
    ../posix/pb_sleep_ms.c                            \
    ../posix/pbtimespec_elapsed_ms.c                  \
    ../posix/posix_socket_blocking_io.c               \
    ../posix/pubnub_generate_uuid_posix.c

# `CORE_SOURCE_FILES` extension for Windows build.
CORE_SOURCE_FILES_WINDOWS = \
    ../core/c99/snprintf.c                            \
    ../windows/msstopwatch_windows.c                  \
    ../windows/pbtimespec_elapsed_ms.c                \
    ../windows/pb_sleep_ms.c                          \
    ../windows/pubnub_generate_uuid_windows.c         \
    ../windows/windows_socket_blocking_io.c

# `CORE_SOURCE_FILES_POSIX` extension for non-CPP build.
CORE_NON_CPP_SOURCE_FILES =

# `CORE_NON_CPP_SOURCE_FILES` extension for non-CPP build.
CORE_NON_CPP_SOURCE_FILES_POSIX = \
    ../posix/pubnub_version_posix.c

# `CORE_NON_CPP_SOURCE_FILES` extension for non-CPP build.
CORE_NON_CPP_SOURCE_FILES_WINDOWS = \
    ../windows/pubnub_version_windows.c

# `CORE_SOURCE_FILES_POSIX` extension for CPP build.
CORE_CPP_SOURCE_FILES = \
    ../cpp/pubnub_subloop.cpp

# `CORE_CPP_SOURCE_FILES` extension for CPP build.
CORE_CPP_SOURCE_FILES_POSIX = \
    ../cpp/pubnub_version_posix.cpp

# `CORE_CPP_SOURCE_FILES` extension for CPP build.
CORE_CPP_SOURCE_FILES_WINDOWS = \
    ../cpp/pubnub_version_windows.cpp

# `CORE_SOURCE_FILES` extension without OpenSSL support used for all
# platforms.
CORE_NON_OPENSSL_SOURCE_FILES = \
    ../lib/md5/md5.c               \
    ../lib/sockets/pbpal_sockets.c

# `CORE_NON_OPENSSL_SOURCE_FILES` extension for POSIX build.
CORE_NON_OPENSSL_SOURCE_FILES_POSIX = \
    ../posix/pbpal_posix_blocking_io.c

# `CORE_NON_OPENSSL_SOURCE_FILES` extension for Windows build.
CORE_NON_OPENSSL_SOURCE_FILES_WINDOWS = \
    ../windows/pbpal_windows_blocking_io.c

# `CORE_SOURCE_FILES` extension with OpenSSL support used for all
# platforms.
CORE_OPENSSL_SOURCE_FILES = \
    ../core/pubnub_ssl.c                   \
    ../openssl/pbpal_connect_openssl.c     \
    ../openssl/pbpal_openssl.c             \
    ../openssl/pbpal_openssl_blocking_io.c

# `CORE_OPENSSL_SOURCE_FILES` extension for POSIX build.
CORE_OPENSSL_SOURCE_FILES_POSIX = \
    ../openssl/pbpal_add_system_certs_posix.c
CORE_OPENSSL_SOURCE_FILES_NOT_DARWIN = \
    ../lib/md5/md5.c

# `CORE_OPENSSL_SOURCE_FILES` extension for Windows build.
CORE_OPENSSL_SOURCE_FILES_WINDOWS = \
    ../openssl/pbpal_add_system_certs_windows.c


###############################################################################
#                       PubNub Core sync source files                         #
###############################################################################

# Core source files for a synchronous PubNub C-core client version support.
SYNC_CORE_SOURCE_FILES = \
    ../core/pubnub_ntf_sync.c            \
    ../core/pubnub_sync_subscribe_loop.c \
    ../core/srand_from_pubnub_time.c


###############################################################################
#                     PubNub Core callback source files                       #
###############################################################################

# Source files for a call-back based PubNub C-core client version support.
CALLBACK_CORE_SOURCE_FILES = \
    ../core/pbpal_ntf_callback_admin.c              \
    ../core/pbpal_ntf_callback_handle_timer_list.c  \
    ../core/pbpal_ntf_callback_queue.c              \
    ../core/pubnub_callback_subscribe_loop.c        \
    ../core/pubnub_timer_list.c                     \
    ../lib/pubnub_dns_codec.c                       \
    ../lib/sockets/pbpal_adns_sockets.c             \
    ../lib/sockets/pbpal_ntf_callback_poller_poll.c

# `CALLBACK_CORE_SOURCE_FILES` extension without OpenSSL support used for all
# platforms.
CALLBACK_CORE_NON_OPENSSL_SOURCE_FILES =

# `CALLBACK_CORE_NON_OPENSSL_SOURCE_FILES` extension for POSIX build.
CALLBACK_CORE_NON_OPENSSL_SOURCE_FILES_POSIX = \
    ../posix/pubnub_get_native_socket.c  \
    ../posix/pubnub_ntf_callback_posix.c

# `CALLBACK_CORE_NON_OPENSSL_SOURCE_FILES` extension for Windows build.
CALLBACK_CORE_NON_OPENSSL_SOURCE_FILES_WINDOWS = \
    ../windows/pubnub_get_native_socket.c    \
    ../windows/pubnub_ntf_callback_windows.c

# `CALLBACK_CORE_SOURCE_FILES` extension with OpenSSL support used for all
# platforms.
CALLBACK_CORE_OPENSSL_SOURCE_FILES = \
    ../openssl/pubnub_get_native_socket.c

# `CALLBACK_CORE_OPENSSL_SOURCE_FILES` extension for POSIX build.
CALLBACK_CORE_OPENSSL_SOURCE_FILES_POSIX = \
    ../openssl/pubnub_ntf_callback_posix.c

# `CALLBACK_CORE_OPENSSL_SOURCE_FILES` extension for Windows build.
CALLBACK_CORE_OPENSSL_SOURCE_FILES_WINDOWS = \
    ../openssl/pubnub_ntf_callback_windows.c

###############################################################################
#                     PubNub SDK NTF runtime api selection                    #
###############################################################################
# PubNub SDK NTF runtime api selection source files.
NTF_RUNTIME_SELECTION_CORE_SOURCE_FILES = \
	../core/pubnub_ntf_enforcement.c 


###############################################################################
#                        PubNub console source files                          #
###############################################################################

CONSOLE_SOURCE_FILES = \
    ../core/samples/console/pnc_helpers.c       \
    ../core/samples/console/pnc_readers.c       \
    ../core/samples/console/pnc_subscriptions.c \
    ../core/samples/console/pubnub_console.c


###############################################################################
#                        PubNub feature source files                          #
###############################################################################

# Published messages content compression feature source files.
GZIP_COMPRESSION_SOURCE_FILES = \
    ../core/pbgzip_compress.c \
    ../lib/miniz/miniz.c      \
    ../lib/miniz/miniz_tdef.c \
    ../lib/pbcrc32.c


# Compressed service response de-compression feature source files.
GZIP_RESPONSE_SOURCE_FILES = \
    ../core/pbgzip_decompress.c \
    ../lib/miniz/miniz_tinfl.c


# Published message reactions feature source files.
MESSAGE_REACTION_SOURCE_FILES = \
    ../core/pbcc_actions_api.c \
    ../core/pubnub_actions_api.c


# Extended history feature source files.
#
# Extended history allows to fetch messages in batch for multiple channels,
# count messages and fetch with additional information.
ADVANCED_HISTORY_SOURCE_FILES = \
    ../core/pbcc_advanced_history.c   \
    ../core/pubnub_advanced_history.c


# Auto heartbeat feature source files.
#
# Feature used together with `USE_SUBSCRIBE_V2` flag to send periodic heartbeat
# requests to update users' presence on subscribed channels.
AUTO_HEARTBEAT_SOURCE_FILES = \
    ../core/pbauto_heartbeat.c      \
    ../lib/pbstr_remove_from_list.c

# `AUTO_HEARTBEAT_SOURCE_FILES` extension without OpenSSL support used for all
# platforms.
AUTO_HEARTBEAT_NON_OPENSSL_SOURCE_FILES =

# `AUTO_HEARTBEAT_NON_OPENSSL_SOURCE_FILES` extension for POSIX build.
AUTO_HEARTBEAT_NON_OPENSSL_SOURCE_FILES_POSIX = \
    ../posix/pbauto_heartbeat_init_posix.c

# `AUTO_HEARTBEAT_NON_OPENSSL_SOURCE_FILES` extension for Windows build.
AUTO_HEARTBEAT_NON_OPENSSL_SOURCE_FILES_WINDOWS = \
    ../windows/pbauto_heartbeat_init_windows.c

# `AUTO_HEARTBEAT_SOURCE_FILES` extension with OpenSSL support used for all
# platforms.
AUTO_HEARTBEAT_OPENSSL_SOURCE_FILES =

# `AUTO_HEARTBEAT_OPENSSL_SOURCE_FILES` extension for POSIX build.
AUTO_HEARTBEAT_OPENSSL_SOURCE_FILES_POSIX = \
    ../openssl/pbauto_heartbeat_init_posix.c

# `AUTO_HEARTBEAT_OPENSSL_SOURCE_FILES` extension for Windows build.
AUTO_HEARTBEAT_OPENSSL_SOURCE_FILES_WINDOWS = \
    ../windows/pbauto_heartbeat_init_windows.c


# Published / received data encryption and decryption feature source files.
CRYPTO_SOURCE_FILES = \
    ../core/pbcc_crypto.c         \
    ../core/pbcc_crypto_aes_cbc.c \
    ../core/pbcc_crypto_legacy.c  \
    ../core/pubnub_crypto.c       \
    ../openssl/pbaes256.c


# Custom DNS servers feature source files.
#
# Feature allows to specify custom DNS servers and send query to them.
# Important: Can be used only together with `PUBNUB_CALLBACK_API` flag.
DNS_SERVERS_SOURCE_FILES = \
    ../core/pubnub_dns_servers.c

# `DNS_SERVERS_SOURCE_FILES` extension for POSIX build.
DNS_SERVERS_SOURCE_FILES_POSIX = \
    ../posix/pubnub_dns_system_servers.c

# `DNS_SERVERS_SOURCE_FILES` extension for Windows build.
DNS_SERVERS_SOURCE_FILES_WINDOWS = \
    ../windows/pubnub_dns_system_servers.c

# Custom DNS servers IPv4 support feature source files.
#
# Important: Can be used only together with `PUBNUB_CALLBACK_API` flag.
IPV4_SOURCE_FILES = \
    ../lib/pubnub_parse_ipv4_addr.c

# Custom DNS servers IPv6 support feature source files.
#
# Important: Can be used only together with `PUBNUB_CALLBACK_API` flag.
IPV6_SOURCE_FILES = \
    ../lib/pubnub_parse_ipv6_addr.c


# Single channel history feature source files.
FETCH_HISTORY_SOURCE_FILES = \
    ../core/pbcc_fetch_history.c   \
    ../core/pubnub_fetch_history.c


# Grant token permissions feature source files.
GRANT_TOKEN_SOURCE_FILES = \
    ../core/pbcc_grant_token_api.c      \
    ../core/pubnub_grant_token_api.c    \
    ../lib/cbor/cborerrorstrings.c      \
    ../lib/cbor/cborparser.c            \
    ../lib/cbor/cborparser_dup_string.c


# App Context feature source files.
APP_CONTEXT_SOURCE_FILES = \
    ../core/pbcc_objects_api.c   \
    ../core/pubnub_objects_api.c


# Custom proxy feature source files.
PROXY_SOURCE_FILES = \
    ../core/pbhttp_digest.c     \
    ../core/pbntlm_core.c       \
    ../core/pbntlm_packer_std.c \
    ../core/pubnub_proxy.c      \
    ../core/pubnub_proxy_core.c

# `PROXY_SOURCE_FILES` extension for POSIX build.
PROXY_SOURCE_FILES_POSIX =

# `PROXY_SOURCE_FILES` extension for Windows build.
PROXY_SOURCE_FILES_WINDOWS = \
    ../windows/pubnub_set_proxy_from_system_windows.c


# Automated failed request retry feature source files.
RETRY_CONFIGURATION_SOURCE_FILES = \
    ../core/pbcc_request_retry_timer.c   \
    ../core/pubnub_retry_configuration.c


# Revoke token permissions feature source files.
REVOKE_TOKEN_SOURCE_FILES = \
    ../core/pbcc_revoke_token_api.c   \
    ../core/pubnub_revoke_token_api.c


# Subscription event engine feature source files.
SUBSCRIBE_EVENT_ENGINE_SOURCE_FILES = \
    ../core/pbcc_event_engine.c                       \
    ../core/pbcc_memory_utils.c                       \
    ../core/pbcc_subscribe_event_engine.c             \
    ../core/pbcc_subscribe_event_engine_effects.c     \
    ../core/pbcc_subscribe_event_engine_events.c      \
    ../core/pbcc_subscribe_event_engine_states.c      \
    ../core/pbcc_subscribe_event_engine_transitions.c \
    ../core/pbcc_subscribe_event_listener.c           \
    ../core/pubnub_entities.c                         \
    ../core/pubnub_subscribe_event_engine.c           \
    ../core/pubnub_subscribe_event_listener.c         \
    ../lib/pbarray.c                                  \
    ../lib/pbhash_set.c                               \
    ../lib/pbref_counter.c                            \
    ../lib/pbstrdup.c


# Subscribe v2 feature source files.
SUBSCRIBE_V2_SOURCE_FILES = \
    ../core/pbcc_subscribe_v2.c   \
    ../core/pubnub_subscribe_v2.c


# Advanced logger core source files.
LOGGER_SOURCE_FILES = \
    ../core/pbcc_logger_manager.c \
    ../core/pubnub_log_value.c    \
    ../core/pubnub_logger.c

# `LOGGER_SOURCE_FILES` extension for default POSIX / Windows logger.
LOGGER_DEFAULT_STDIO_SOURCE_FILES = \
    ../core/pubnub_stdio_logger.c
