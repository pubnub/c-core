# CMakeLists.txt for the C-core library of Pubnub.
cmake_minimum_required(VERSION 3.12)

# Setup `try_compile` if required.
if (DEFINED SHARED_LIB)
    # Setup try_compile to build for static library.
    if (NOT ${SHARED_LIB})
        set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
    endif ()
endif ()

# Project name and version for casual builds
# For ESP32 builds it is considered as a IDF component
if (NOT ESP_PLATFORM)
    project(pubnub VERSION 4.10.0)
endif ()

message(STATUS Preparing\ ${PROJECT_NAME}\ version\ ${PROJECT_VERSION})

# Function to convert between "ON"/"OFF" and 1/0
function(num_option name description default)
    option(${name} ${description} ${default})
    if (${name})
        message(STATUS "[*] ${name}")
        set(${name} 1 PARENT_SCOPE)
    else ()
        message(STATUS "[ ] ${name}")
        set(${name} 0 PARENT_SCOPE)
    endif ()
endfunction()

# Function to log a option and its value
function(log_option name description default)
    option(${name} ${description} ${default})
    if (${name})
        message(STATUS "[*] ${name}")
    else ()
        message(STATUS "[ ] ${name}")
    endif ()
endfunction()

# Function to log a set and its value
function(log_set name default description)
    set(${name} ${default} CACHE STRING ${description})
    message(STATUS "${name}:${${name}}")
endfunction()

# All the options that can be set on the command line to control

message(STATUS "Options (Enabled *):")

log_option(OPENSSL "Use OpenSSL" OFF)
num_option(MBEDTLS "Use mbedTLS" OFF)
log_option(EXAMPLES "Build all examples" OFF)
log_option(SHARED_LIB "Library type. [SHARED=ON STATIC=OFF]" OFF)
log_option(WITH_CPP "Build the CPP headers" OFF)
log_option(ASAN "Use Address sanitizer" OFF)

if (WITH_CPP)
    set(DEFAULT_USE_CALLBACK_API ON)
else ()
    set(DEFAULT_USE_CALLBACK_API OFF)
endif ()

log_option(USE_CPP11 "Use C++11 [WITH_CPP=ON needed]" OFF)
log_option(UNIT_TEST "Build and run unit tests" OFF)
num_option(ONLY_PUBSUB_API "Only pubsub API" OFF)
num_option(USE_PROXY "Use proxy" ON)
num_option(USE_GZIP_COMPRESSION "Use gzip compression" ON)
num_option(RECEIVE_GZIP_RESPONSE "Use gzip decompression" ON)
num_option(USE_RETRY_CONFIGURATION "Use requests retry" OFF)
num_option(USE_SUBSCRIBE_V2 "Use subscribe v2" ON)
num_option(USE_SUBSCRIBE_EVENT_ENGINE "Use Subscribe Event Engine [USE_SUBSCRIBE_V2=ON needed]" OFF)
num_option(USE_ADVANCED_HISTORY "Use advanced history" ON)
num_option(USE_OBJECTS_API "Use objects API" ON)
num_option(USE_AUTO_HEARTBEAT "Use auto heartbeat" ON)
num_option(USE_ACTIONS_API "Use actions API" ON)
num_option(USE_REVOKE_TOKEN_API "Use revoke token API [OPENSSL ONLY]" OFF)
num_option(USE_GRANT_TOKEN_API "Use grant token API [OPENSSL ONLY]" OFF)
num_option(USE_FETCH_HISTORY "Use fetch history" ON)
num_option(USE_CRYPTO_API "Use crypto API [OPENSSL ONLY]" OFF)
num_option(USE_CALLBACK_API "Use callback API [CALLBACK=ON SYNC=OFF]" ${DEFAULT_USE_CALLBACK_API})
num_option(USE_IPV6 "Use IPv6" ON)
num_option(USE_SET_DNS_SERVERS "Use set DNS servers [CALLBACK=ON]" ${DEFAULT_USE_CALLBACK_API})
num_option(USE_EXTERN_API "Use extern C API [WITH_CPP=ON]" ON)
num_option(USE_LEGACY_CRYPTO_RANDOM_IV "Use random IV for legacy crypto module [OpenSSL only]" ON)
num_option(USE_LOG_CALLBACK "Use possibility to replace default logging function with user provided callback" OFF)
log_set(OPENSSL_ROOT_DIR "" "OpenSSL root directory (leave empty for find_package() defaults)[OPENSSL=ON needed]")
log_set(CUSTOM_OPENSSL_LIB_DIR "lib" "OpenSSL lib directory relative to OPENSSL_ROOT_DIR [used only if find_package() failed]")
log_set(CUSTOM_OPENSSL_INCLUDE_DIR "include" "OpenSSL include directory relative to OPENSSL_ROOT_DIR [used only if find_package() failed]")
log_set(EXAMPLE "all" "Build example with provided name (use 'all' for all examples) [EXAMPLES=ON needed]")
log_set(CGREEN_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/cgreen" "CGreen root directory [UNIT_TEST=ON needed]")
log_set(LOG_LEVEL "WARNING" "Log level [TRACE/DEBUG/INFO/WARNING/ERROR]")
log_set(CUSTOM_BOOL_TYPE "" "Type of bool for platform differences. Select whatever works for you that accepts 0/1 values")

if (${OPENSSL} AND ${MBEDTLS})
    message(FATAL_ERROR "You can't use both OpenSSL and mbedTLS at the same time!")
endif ()

# Flags configuration

set(FLAGS "\
    -Wall \
    -D PUBNUB_THREADSAFE \
    -D PUBNUB_LOG_LEVEL=PUBNUB_LOG_LEVEL_${LOG_LEVEL} \
    -D PUBNUB_ONLY_PUBSUB_API=${ONLY_PUBSUB_API} \
    -D PUBNUB_PROXY_API=${USE_PROXY} \
    -D PUBNUB_USE_GZIP_COMPRESSION=${USE_GZIP_COMPRESSION} \
    -D PUBNUB_RECEIVE_GZIP_RESPONSE=${RECEIVE_GZIP_RESPONSE} \
    -D PUBNUB_USE_RETRY_CONFIGURATION=${USE_RETRY_CONFIGURATION} \
    -D PUBNUB_USE_SUBSCRIBE_V2=${USE_SUBSCRIBE_V2} \
    -D PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE=${USE_SUBSCRIBE_EVENT_ENGINE} \
    -D PUBNUB_USE_OBJECTS_API=${USE_OBJECTS_API} \
    -D PUBNUB_USE_ACTIONS_API=${USE_ACTIONS_API} \
    -D PUBNUB_USE_AUTO_HEARTBEAT=${USE_AUTO_HEARTBEAT} \
    -D PUBNUB_USE_GRANT_TOKEN_API=${USE_GRANT_TOKEN_API} \
    -D PUBNUB_USE_REVOKE_TOKEN_API=${USE_REVOKE_TOKEN_API} \
    -D PUBNUB_USE_FETCH_HISTORY=${USE_FETCH_HISTORY} \
    -D PUBNUB_CRYPTO_API=${USE_CRYPTO_API} \
    -D PUBNUB_RAND_INIT_VECTOR=${USE_LEGACY_CRYPTO_RANDOM_IV} \
    -D PUBNUB_USE_LOG_CALLBACK=${USE_LOG_CALLBACK} \
    -D PUBNUB_MBEDTLS=${MBEDTLS}")

if (DEFINED SDK_VERSION_SUFFIX)
    set(FLAGS "\
        ${FLAGS} \
        -D PUBNUB_SDK_VERSION_SUFFIX=${SDK_VERSION_SUFFIX}")
endif ()

#Required on windows with Event Engine, so only winsock2 is used (otherwise it tries to use winsock and winsock2 and there are redefinition errors)
if (WIN32 OR WIN64 OR MSVC)
    set(FLAGS "\
    ${FLAGS} \
	-D _WINSOCKAPI_")
endif ()


set(LDLIBS)
set(OS_SOURCEFILES)

if (${USE_CALLBACK_API})
    set(FLAGS "\
        ${FLAGS} \
        -D PUBNUB_SET_DNS_SERVERS=${USE_SET_DNS_SERVERS} \
        -D PUBNUB_USE_IPV6=${USE_IPV6} \
        -D PUBNUB_CALLBACK_API")
endif ()

if (${ASAN})
    set(FLAGS "\
        ${FLAGS} \
        -g \
        -fsanitize=address")
    set(LDLIBS "\
        ${LDLIBS} \
        -fsanitize=address")
endif ()

set(CMAKE_C_FLAGS "\
    ${FLAGS} \
    ${CMAKE_C_FLAGS}")

if (${WITH_CPP})
    set(CMAKE_CXX_FLAGS "\
        ${FLAGS} \
        -D PUBNUB_USE_EXTERN_C=${USE_EXTERN_API} \
        ${CMAKE_CXX_FLAGS}")

    if (${USE_CPP11})
        set(CMAKE_CXX_FLAGS "\
            ${CMAKE_CXX_FLAGS} \
            -std=c++11")
    endif ()
endif ()

if (NOT ${CUSTOM_BOOL_TYPE} STREQUAL "")
    message(STATUS "Using custom bool type: ${CUSTOM_BOOL_TYPE}")
    set(FLAGS "\
        ${FLAGS} \
        -D PUBNUB_BOOL_TYPE=${CUSTOM_BOOL_TYPE}")
endif ()

set(CORE_SOURCEFILES
        ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_set_state.c
        ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_pubsubapi.c
        ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_coreapi.c
        ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_coreapi_ex.c
        ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_ccore_pubsub.c
        ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_ccore.c
        ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_netcore.c
        ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_blocking_io.c
        ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_timers.c
        ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_log.c
        ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_json_parse.c
        ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_helper.c
        ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_generate_uuid_v3_md5.c
        ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_free_with_timeout_std.c
        ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_url_encode.c)

set(LIB_SOURCEFILES
        ${CMAKE_CURRENT_LIST_DIR}/lib/sockets/pbpal_handle_socket_error.c
        ${CMAKE_CURRENT_LIST_DIR}/lib/base64/pbbase64.c
        ${CMAKE_CURRENT_LIST_DIR}/lib/pb_strnlen_s.c
        ${CMAKE_CURRENT_LIST_DIR}/lib/pb_strncasecmp.c
        ${CMAKE_CURRENT_LIST_DIR}/lib/pubnub_dns_codec.c)

if (UNIX OR WIN32 OR WIN64 OR MSVC)
    set(CORE_SOURCEFILES
            ${CORE_SOURCEFILES}
            ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_alloc_std.c
            ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_generate_uuid.c
            ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_assert_std.c)

    set(LIB_SOURCEFILES
            ${LIB_SOURCEFILES}
            ${CMAKE_CURRENT_LIST_DIR}/lib/sockets/pbpal_adns_sockets.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/sockets/pbpal_resolv_and_connect_sockets.c)
else ()
    set(CORE_SOURCEFILES
            ${CORE_SOURCEFILES}
            ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_alloc_std.c)
endif ()

if (NOT ${OPENSSL})
    set(LIB_SOURCEFILES
            ${LIB_SOURCEFILES}
            ${CMAKE_CURRENT_LIST_DIR}/lib/md5/md5.c)
endif ()

if (UNIX)
    message(STATUS "Using POSIX setup")

    set(LDLIBS "-lpthread ${LDLIBS}")
    set(OS_SOURCEFILES
            ${CMAKE_CURRENT_LIST_DIR}/posix/posix_socket_blocking_io.c
            ${CMAKE_CURRENT_LIST_DIR}/posix/pubnub_version_posix.c
            ${CMAKE_CURRENT_LIST_DIR}/posix/pubnub_generate_uuid_posix.c
            ${CMAKE_CURRENT_LIST_DIR}/posix/msstopwatch_monotonic_clock.c
            ${CMAKE_CURRENT_LIST_DIR}/posix/pbtimespec_elapsed_ms.c
            ${CMAKE_CURRENT_LIST_DIR}/posix/pb_sleep_ms.c)

    if (APPLE)
        set(OS_SOURCEFILES ${OS_SOURCEFILES} ${CMAKE_CURRENT_LIST_DIR}/posix/monotonic_clock_get_time_darwin.c)
    else ()
        set(OS_SOURCEFILES ${OS_SOURCEFILES} ${CMAKE_CURRENT_LIST_DIR}/posix/monotonic_clock_get_time_posix.c)
    endif ()
elseif (WIN32 OR WIN64 OR MSVC)
    message(STATUS "Using Windows setup")

    set(LDLIBS "ws2_32.lib IPHlpAPI.lib rpcrt4.lib")
    set(OS_SOURCEFILES
            ${CMAKE_CURRENT_LIST_DIR}/windows/windows_socket_blocking_io.c
            ${CMAKE_CURRENT_LIST_DIR}/windows/pbtimespec_elapsed_ms.c
            ${CMAKE_CURRENT_LIST_DIR}/windows/pubnub_version_windows.c
            ${CMAKE_CURRENT_LIST_DIR}/windows/pubnub_generate_uuid_windows.c
            ${CMAKE_CURRENT_LIST_DIR}/windows/pbpal_windows_blocking_io.c
            ${CMAKE_CURRENT_LIST_DIR}/windows/msstopwatch_windows.c
            ${CMAKE_CURRENT_LIST_DIR}/windows/pb_sleep_ms.c
            ${CMAKE_CURRENT_LIST_DIR}/windows/pbauto_heartbeat_init_windows.c)
else ()
    message(STATUS "Using FreeRTOS setup")

    set(OS_SOURCEFILES
            ${CMAKE_CURRENT_LIST_DIR}/freertos/pbpal_freertos_blocking_io.c
            ${CMAKE_CURRENT_LIST_DIR}/freertos/pbpal_resolv_and_connect_freertos_tcp.c
            ${CMAKE_CURRENT_LIST_DIR}/freertos/pubnub_assert_freertos.c
            ${CMAKE_CURRENT_LIST_DIR}/freertos/pubnub_generate_uuid_freertos.c
            ${CMAKE_CURRENT_LIST_DIR}/freertos/pubnub_version_freertos.c)
    if (ESP_PLATFORM)
        set(OS_SOURCEFILES ${OS_SOURCEFILES}
                ${CMAKE_CURRENT_LIST_DIR}/freertos/msstopwatch_esp32.c
                ${CMAKE_CURRENT_LIST_DIR}/freertos/pbtimespec_elapsed_ms_esp32.c)
    endif ()
endif ()

set(INTF_SOURCEFILES)

if (${USE_CALLBACK_API})
    message(STATUS "Using callback API")

    set(CORE_SOURCEFILES
            ${CORE_SOURCEFILES}
            ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_memory_block.c
            ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_dns_servers.c)

    set(INTF_SOURCEFILES
            ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_timer_list.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/pubnub_parse_ipv4_addr.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/pubnub_parse_ipv6_addr.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/sockets/pbpal_ntf_callback_poller_poll.c
            ${CMAKE_CURRENT_LIST_DIR}/core/pbpal_ntf_callback_queue.c
            ${CMAKE_CURRENT_LIST_DIR}/core/pbpal_ntf_callback_handle_timer_list.c
            ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_callback_subscribe_loop.c
            ${CMAKE_CURRENT_LIST_DIR}/core/pbpal_ntf_callback_admin.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/sockets/pbpal_adns_sockets.c
            ${INTF_SOURCEFILES})

    if (UNIX)
        set(INTF_SOURCEFILES
                ${INTF_SOURCEFILES}
                ${CMAKE_CURRENT_LIST_DIR}/posix/pubnub_dns_system_servers.c
                ${CMAKE_CURRENT_LIST_DIR}/posix/pubnub_ntf_callback_posix.c
                ${CMAKE_CURRENT_LIST_DIR}/posix/pubnub_get_native_socket.c)
    elseif (WIN32 OR WIN64 OR MSVC)
        set(INTF_SOURCEFILES
                ${INTF_SOURCEFILES}
                ${CMAKE_CURRENT_LIST_DIR}/windows/pubnub_ntf_callback_windows.c
                ${CMAKE_CURRENT_LIST_DIR}/windows/pubnub_dns_system_servers.c
                ${CMAKE_CURRENT_LIST_DIR}/windows/pubnub_get_native_socket.c)
    endif ()
else ()
    message(STATUS "Using sync API")

    set(INTF_SOURCEFILES
            ${INTF_SOURCEFILES}
            ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_ntf_sync.c
            ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_sync_subscribe_loop.c
            ${CMAKE_CURRENT_LIST_DIR}/core/srand_from_pubnub_time.c)
endif ()

set(FEATURE_SOURCEFILES)

if (${USE_PROXY})
    set(FEATURE_SOURCEFILES
            ${FEATURE_SOURCEFILES}
            ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_proxy.c
            ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_proxy_core.c
            ${CMAKE_CURRENT_LIST_DIR}/core/pbhttp_digest.c
            ${CMAKE_CURRENT_LIST_DIR}/core/pbntlm_core.c
            ${CMAKE_CURRENT_LIST_DIR}/core/pbntlm_packer_std.c)
endif ()

if (${USE_GZIP_COMPRESSION})
    set(FEATURE_SOURCEFILES
            ${FEATURE_SOURCEFILES}
            ${CMAKE_CURRENT_LIST_DIR}/lib/miniz/miniz_tdef.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/miniz/miniz.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/pbcrc32.c
            ${CMAKE_CURRENT_LIST_DIR}/core/pbgzip_compress.c)
endif ()

if (${RECEIVE_GZIP_RESPONSE})
    set(FEATURE_SOURCEFILES
            ${FEATURE_SOURCEFILES}
            ${CMAKE_CURRENT_LIST_DIR}/lib/miniz/miniz_tinfl.c
            ${CMAKE_CURRENT_LIST_DIR}/core/pbgzip_decompress.c)
endif ()

if (${USE_RETRY_CONFIGURATION})
    set(FEATURE_SOURCEFILES
            ${FEATURE_SOURCEFILES}
            ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_request_retry_timer.c
            ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_retry_configuration.c)
endif ()

if (${USE_SUBSCRIBE_V2})
    set(FEATURE_SOURCEFILES
            ${FEATURE_SOURCEFILES}
            ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_subscribe_v2.c
            ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_subscribe_v2.c)
endif ()

if (${USE_SUBSCRIBE_EVENT_ENGINE})
    message(STATUS "Using subscribe event engine API")
    set(LIB_SOURCEFILES
            ${LIB_SOURCEFILES}
            ${CMAKE_CURRENT_LIST_DIR}/lib/pbarray.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/pbhash_set.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/pbref_counter.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/pbstrdup.c)
    set(FEATURE_SOURCEFILES
            ${FEATURE_SOURCEFILES}
            ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_memory_utils.c
            ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_event_engine.c
            ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_subscribe_event_engine.c
            ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_subscribe_event_engine_states.c
            ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_subscribe_event_engine_events.c
            ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_subscribe_event_engine_effects.c
            ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_subscribe_event_engine_transitions.c
            ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_subscribe_event_listener.c
            ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_entities.c
            ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_subscribe_event_engine.c
            ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_subscribe_event_listener.c)
endif ()

if (${USE_ADVANCED_HISTORY})
    set(FEATURE_SOURCEFILES
            ${FEATURE_SOURCEFILES}
            ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_advanced_history.c
            ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_advanced_history.c)
endif ()

if (${USE_OBJECTS_API})
    set(FEATURE_SOURCEFILES
            ${FEATURE_SOURCEFILES}
            ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_objects_api.c
            ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_objects_api.c)
endif ()

if (${USE_ACTIONS_API})
    set(FEATURE_SOURCEFILES
            ${FEATURE_SOURCEFILES}
            ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_actions_api.c
            ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_actions_api.c)
endif ()

if (${USE_AUTO_HEARTBEAT})
    set(FEATURE_SOURCEFILES
            ${FEATURE_SOURCEFILES}
            ${CMAKE_CURRENT_LIST_DIR}/core/pbauto_heartbeat.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/pbstr_remove_from_list.c)
    if (NOT ${OPENSSL})
        if (UNIX)
            set(FEATURE_SOURCEFILES
                    ${FEATURE_SOURCEFILES}
                    ${CMAKE_CURRENT_LIST_DIR}/posix/pbauto_heartbeat_init_posix.c)
        elseif (WIN32 OR WIN64 OR MSVC)
            set(FEATURE_SOURCEFILES
                    ${FEATURE_SOURCEFILES}
                    ${CMAKE_CURRENT_LIST_DIR}/windows/pbauto_heartbeat_init_windows.c)
        endif ()
    endif ()
endif ()

if (${USE_FETCH_HISTORY})
    set(FEATURE_SOURCEFILES
            ${FEATURE_SOURCEFILES}
            ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_fetch_history.c
            ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_fetch_history.c)
endif ()

if (${USE_GRANT_TOKEN_API})
    set(FEATURE_SOURCEFILES
            ${FEATURE_SOURCEFILES}
            ${CMAKE_CURRENT_LIST_DIR}/lib/cbor/cborparser.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/cbor/cborerrorstrings.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/cbor/cborparser_dup_string.c
            ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_grant_token_api.c
            ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_grant_token_api.c)
endif ()

if (${USE_REVOKE_TOKEN_API})
    set(FEATURE_SOURCEFILES
            ${FEATURE_SOURCEFILES}
            ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_revoke_token_api.c
            ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_revoke_token_api.c)
endif ()

set(SSL_LINK_LIBRARIES)
if (${OPENSSL})
    message(STATUS "Using OpenSSL")

    if ("${OPENSSL_ROOT_DIR}" STREQUAL "")
        find_package(OpenSSL REQUIRED)
        set(SSL_LINK_LIBRARIES OpenSSL::SSL OpenSSL::Crypto)
    else ()
        find_package(OpenSSL PATHS ${OPENSSL_ROOT_DIR})
        if (OpenSSl_FOUND)
            set(SSL_LINK_LIBRARIES OpenSSL::SSL OpenSSL::Crypto)
        else ()
            message(STATUS "Searching for custom OpenSSL library")
            if (WIN32 OR WIN64 OR MSVC)
                find_library(OpenSSL REQUIRED NAMES libssl PATHS ${OPENSSL_ROOT_DIR}/${CUSTOM_OPENSSL_LIB_DIR} NO_DEFAULT_PATH)
                find_library(Crypto REQUIRED NAMES libcrypto PATHS ${OPENSSL_ROOT_DIR}/${CUSTOM_OPENSSL_LIB_DIR} NO_DEFAULT_PATH)
            elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
                # find_library doesn't work when building for Android architecture
                set(OpenSSL "${OPENSSL_ROOT_DIR}/${CUSTOM_OPENSSL_LIB_DIR}/libssl.a")
                set(Crypto "${OPENSSL_ROOT_DIR}/${CUSTOM_OPENSSL_LIB_DIR}/libcrypto.a")
                link_libraries(${OpenSSL} ${Crypto})
            else ()
                find_library(OpenSSL REQUIRED NAMES ssl PATHS ${OPENSSL_ROOT_DIR}/${CUSTOM_OPENSSL_LIB_DIR} NO_DEFAULT_PATH)
                find_library(Crypto REQUIRED NAMES crypto PATHS ${OPENSSL_ROOT_DIR}/${CUSTOM_OPENSSL_LIB_DIR} NO_DEFAULT_PATH)
            endif ()

            message(STATUS "Used libraries:")
            message(STATUS "-> ${OpenSSL}")
            message(STATUS "-> ${Crypto}")

            set(SSL_LINK_LIBRARIES ${OpenSSL} ${Crypto})
            include_directories(${OPENSSL_ROOT_DIR}/${CUSTOM_OPENSSL_INCLUDE_DIR})

            message(STATUS "Used headers:")
            message(STATUS "-> ${OPENSSL_ROOT_DIR}/${CUSTOM_OPENSSL_INCLUDE_DIR}")

            message(STATUS "OpenSSL ready!")
        endif ()
    endif ()

    set(FEATURE_SOURCEFILES
            ${OPENSSL_LIBRARIES}
            ${FEATURE_SOURCEFILES})

    set(LIB_SOURCEFILES
            ${CMAKE_CURRENT_LIST_DIR}/lib/base64/pbbase64.c
            ${LIB_SOURCEFILES})

    set(FEATURE_SOURCEFILES
            ${CMAKE_CURRENT_LIST_DIR}/openssl/pbpal_openssl.c
            ${CMAKE_CURRENT_LIST_DIR}/openssl/pbpal_connect_openssl.c
            ${CMAKE_CURRENT_LIST_DIR}/openssl/pbpal_openssl_blocking_io.c
            ${FEATURE_SOURCEFILES})

    if (UNIX)
        set(FEATURE_SOURCEFILES
                ${CMAKE_CURRENT_LIST_DIR}/posix/pbauto_heartbeat_init_posix.c
                ${FEATURE_SOURCEFILES})
    else ()
        set(FEATURE_SOURCEFILES
                ${CMAKE_CURRENT_LIST_DIR}/windows/pbauto_heartbeat_init_windows.c
                ${FEATURE_SOURCEFILES})
    endif ()

    set(LDLIBS ${LDLIBS})

    if (USE_CRYPTO_API)
        set(FEATURE_SOURCEFILES
                ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_crypto.c
                ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_crypto_aes_cbc.c
                ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_crypto_legacy.c
                ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_crypto.c
                ${CMAKE_CURRENT_LIST_DIR}/openssl/pbaes256.c
                ${FEATURE_SOURCEFILES})
    endif ()

    if (UNIX)
        set(FEATURE_SOURCEFILES ${FEATURE_SOURCEFILES} ${CMAKE_CURRENT_LIST_DIR}/openssl/pbpal_add_system_certs_posix.c)
    elseif (WIN32 OR WIN64 OR MSVC)
        set(FEATURE_SOURCEFILES ${FEATURE_SOURCEFILES} ${CMAKE_CURRENT_LIST_DIR}/openssl/pbpal_add_system_certs_windows.c)
    endif ()
elseif (${MBEDTLS})
    message(STATUS "Using mbedTLS")

    if (NOT ESP_PLATFORM)
        # TODO: Add mbedTLS support for generic builds
    endif ()

    set(FEATURE_SOURCEFILES ${FEATURE_SOURCEFILES}
            ${CMAKE_CURRENT_LIST_DIR}/mbedtls/pbpal_mbedtls.c
            ${CMAKE_CURRENT_LIST_DIR}/mbedtls/pbpal_connect_mbedtls.c)
else ()
    set(LIB_SOURCEFILES
            ${CMAKE_CURRENT_LIST_DIR}/lib/sockets/pbpal_sockets.c
            ${LIB_SOURCEFILES})
    if (UNIX)
        set(LIB_SOURCEFILES
                ${CMAKE_CURRENT_LIST_DIR}/posix/pbpal_posix_blocking_io.c
                ${LIB_SOURCEFILES})
    elseif (WIN32 OR WIN64 OR MSVC)
        set(LIB_SOURCEFILES
                ${CMAKE_CURRENT_LIST_DIR}/windows/pbpal_windows_blocking_io.c
                ${LIB_SOURCEFILES})
    endif ()
endif ()

set(SOURCEFILES
        ${CORE_SOURCEFILES}
        ${LIB_SOURCEFILES}
        ${OS_SOURCEFILES}
        ${FEATURE_SOURCEFILES}
        ${INTF_SOURCEFILES})

if (NOT ESP_PLATFORM)
    if (${SHARED_LIB})
        message(STATUS "Building shared library")
        set(LIBTYPE SHARED)
    else ()
        message(STATUS "Building static library")
        set(LIBTYPE STATIC)
    endif ()

    add_library(pubnub ${LIBTYPE} ${SOURCEFILES})

    if (NOT ${CUSTOM_BOOL_TYPE} STREQUAL "")
        target_compile_definitions(pubnub PUBLIC PUBNUB_BOOL_TYPE=${CUSTOM_BOOL_TYPE})
    endif ()

    target_include_directories(pubnub PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}
            ${CMAKE_CURRENT_LIST_DIR}/core
            ${CMAKE_CURRENT_LIST_DIR}/lib)

    if (${WITH_CPP})
        target_include_directories(pubnub PUBLIC ${CMAKE_CURRENT_LIST_DIR}/cpp)
    endif ()

    if (${OPENSSL})
        target_include_directories(pubnub PUBLIC
                ${CMAKE_CURRENT_LIST_DIR}/openssl/
                ${CMAKE_CURRENT_LIST_DIR}/lib/base64)
        target_include_directories(pubnub PUBLIC ${OPENSSL_INCLUDE_DIR})
    elseif (${MBEDTLS})
        target_include_directories(pubnub PUBLIC ${CMAKE_CURRENT_LIST_DIR}/mbedtls)
    else ()
        if (UNIX)
            target_include_directories(pubnub PUBLIC ${CMAKE_CURRENT_LIST_DIR}/posix)
        elseif (WIN32 OR WIN64 OR MSVC)
            target_include_directories(pubnub PUBLIC
                    ${CMAKE_CURRENT_LIST_DIR}/windows
                    ${CMAKE_CURRENT_LIST_DIR}/core/c99)
        else ()
            target_include_directories(pubnub PUBLIC ${CMAKE_CURRENT_LIST_DIR}/freertos)
        endif ()
    endif ()

    string(STRIP ${LDLIBS} LDLIBS)
    target_link_libraries(pubnub ${LDLIBS} ${SSL_LINK_LIBRARIES})
else ()
    message(STATUS "Building ESP32 IDF component")

    set(EXTRA_CFLAGS ${EXTRA_CFLAGS} ${FLAGS})

    if (${MBEDTLS})
        idf_component_register(SRCS ${SOURCEFILES}
                INCLUDE_DIRS
                ${CMAKE_CURRENT_LIST_DIR}
                ${CMAKE_CURRENT_LIST_DIR}/core
                ${CMAKE_CURRENT_LIST_DIR}/lib
                ${CMAKE_CURRENT_LIST_DIR}/freertos
                ${CMAKE_CURRENT_LIST_DIR}/mbedtls
                $ENV{IDF_PATH}/components/freertos/FreeRTOS-Kernel/include/freertos
                $ENV{IDF_PATH}/components/mbedtls/mbedtls/include
                REQUIRES
                freertos
                lwip
                mbedtls)
    else ()
        idf_component_register(SRCS ${SOURCEFILES}
                INCLUDE_DIRS
                ${CMAKE_CURRENT_LIST_DIR}
                ${CMAKE_CURRENT_LIST_DIR}/core
                ${CMAKE_CURRENT_LIST_DIR}/lib
                ${CMAKE_CURRENT_LIST_DIR}/freertos
                $ENV{IDF_PATH}/components/freertos/FreeRTOS-Kernel/include/freertos
                REQUIRES
                freertos
                lwip)
    endif ()
endif ()

if (NOT ESP_PLATFORM)

endif ()

# TODO: C++ support for ESP32 platform
if (WITH_CPP AND ESP_PLATFORM)
    message(FATAL_ERROR "C++ support is not available for ESP32 platform!")
endif ()

if (WITH_CPP AND NOT ESP_PLATFORM)
    # TODO: Tests
    # fntest/pubnub_fntest_medium.cpp
    # fntest/pubnub_fntest_runner.cpp
    # fntest/pubnub_fntest_basic.cpp
    # fntest/pubnub_fntest.cpp

    set(CPP_SOURCE
            ${CMAKE_CURRENT_LIST_DIR}/cpp/pubnub_subloop.cpp)

    if (UNIX)
        set(CPP_SOURCE
                ${CPP_SOURCE}
                ${CMAKE_CURRENT_LIST_DIR}/cpp/pubnub_version_posix.cpp)

        if (${USE_CPP11})
            set(CPP_SOURCE
                    ${CPP_SOURCE}
                    ${CMAKE_CURRENT_LIST_DIR}/cpp/pubnub_futres_cpp11.cpp)
        else ()
            set(CPP_SOURCE
                    ${CPP_SOURCE}
                    ${CMAKE_CURRENT_LIST_DIR}/cpp/pubnub_futres_posix.cpp)
        endif ()
    endif ()

    if (WIN32 OR WIN64 OR MSVC)
        set(CPP_SOURCE
                ${CPP_SOURCE}
                ${CMAKE_CURRENT_LIST_DIR}/cpp/pubnub_version_windows.cpp)

        if (${USE_CPP11})
            set(CPP_SOURCE
                    ${CPP_SOURCE}
                    ${CMAKE_CURRENT_LIST_DIR}/cpp/pubnub_futres_cpp11.cpp)
        else ()
            set(CPP_SOURCE
                    ${CPP_SOURCE}
                    ${CMAKE_CURRENT_LIST_DIR}/cpp/pubnub_futres_windows.cpp)
        endif ()
    endif ()

    add_library(pubnub-cpp ${LIBTYPE} ${CPP_SOURCE})
    target_link_libraries(pubnub-cpp PUBLIC pubnub)
    target_include_directories(pubnub-cpp PUBLIC ${CMAKE_CURRENT_LIST_DIR}/cpp)
endif ()

if (${EXAMPLES})
    message(STATUS "Building all examples")
    set(EXAMPLE_LIST)
    set(CPP_EXAMPLE_LIST)


    if (${EXAMPLE} STREQUAL "all")
        if (WITH_CPP)
            set(CPP_EXAMPLE_LIST
                    pubnub_subloop_sample
                    pubnub_sample
                    cancel_subscribe_sync_sample
                    futres_nesting
                    ${CPP_EXAMPLE_LIST})

            if (${OPENSSL} AND ${USE_CPP11})
                # TODO: Fix this example to be 98 compatible
                set(CPP_EXAMPLE_LIST
                        pubnub_crypto_module_sample
                        ${CPP_EXAMPLE_LIST})
            endif ()

            if (${USE_GRANT_TOKEN_API})
                set(CPP_EXAMPLE_LIST
                        pubnub_sample_grant_token
                        ${CPP_EXAMPLE_LIST})
            endif ()

            if (${USE_REVOKE_TOKEN_API})
                set(CPP_EXAMPLE_LIST
                        pubnub_sample_revoke_token
                        ${CPP_EXAMPLE_LIST})
            endif ()
        endif ()

        if (${USE_CALLBACK_API})
            message(STATUS "Building callback examples")
            set(EXAMPLE_LIST
                    pubnub_callback_sample
                    pubnub_callback_subloop_sample
                    subscribe_publish_callback_sample
                    subscribe_publish_from_callback
                    publish_callback_subloop_sample
                    publish_queue_callback_subloop)
            if (${USE_SUBSCRIBE_EVENT_ENGINE})
                set(EXAMPLE_LIST
                        subscribe_event_engine_sample
                        ${EXAMPLE_LIST})
            endif ()
            if (WITH_CPP)
                set(CPP_EXAMPLE_LIST
                        subscribe_publish_callback_sample # Only supports callback!
                        ${CPP_EXAMPLE_LIST})
            endif ()
        else ()
            message(STATUS "Building sync examples")
            set(EXAMPLE_LIST
                    metadata
                    pubnub_sync_subloop_sample
                    pubnub_sync_publish_retry
                    pubnub_publish_via_post_sample
                    pubnub_advanced_history_sample
                    pubnub_fetch_history_sample
                    cancel_subscribe_sync_sample)
            if (OPENSSL)
                set(EXAMPLE_LIST
                        pubnub_crypto_module_sample
                        ${EXAMPLE_LIST})
                if (${USE_GRANT_TOKEN_API})
                    set(EXAMPLE_LIST
                            pubnub_sync_grant_token_sample
                            ${EXAMPLE_LIST})
                endif ()
            endif ()
            if (${USE_OBJECTS_API})
                set(EXAMPLE_LIST
                        pubnub_objects_api_sample
                        ${EXAMPLE_LIST})
            endif ()
        endif ()
    else ()
        message(STATUS "Building example ${EXAMPLE}")
        set(EXAMPLE_LIST ${EXAMPLE})
    endif ()

    foreach (EXAMPLE ${EXAMPLE_LIST})
        add_executable(${EXAMPLE} ${CMAKE_CURRENT_LIST_DIR}/core/samples/${EXAMPLE}.c)
        target_link_libraries(${EXAMPLE} pubnub ${LDLIBS})
    endforeach ()

    foreach (EXAMPLE ${CPP_EXAMPLE_LIST})
        add_executable(${EXAMPLE}-cpp ${CMAKE_CURRENT_LIST_DIR}/cpp/samples/${EXAMPLE}.cpp)
        target_link_libraries(${EXAMPLE}-cpp pubnub-cpp ${LDLIBS})
    endforeach ()
endif ()

if (${UNIT_TEST})
    message(STATUS "Building unit tests")
    message(STATUS "UNIT TESTS ARE NOT FINISHED YET!")

    #    set(CMAKE_C_FLAGS " \
    #        ${CMAKE_C_FLAGS} \
    #        -D PUBNUB_ADVANCED_KEEP_ALIVE=1 \
    #        -D PUBNUB_DYNAMIC_REPLY_BUFFER=1 \
    #        -D PUBNUB_ORIGIN_SETTABLE=1")
    #
    #    add_library(pubnub-unit-test SHARED core/pubnub_core_unit_test.c core/test/pubnub_test_mocks.c ${SOURCEFILES})
    #    target_include_directories(pubnub-unit-test PUBLIC
    #        ${CGREEN_ROOT_DIR}/include
    #        ${CMAKE_CURRENT_LIST_DIR}/core
    #        ${CMAKE_CURRENT_LIST_DIR}/core/test
    #        ${CMAKE_CURRENT_LIST_DIR}/lib
    #        ${CMAKE_CURRENT_LIST_DIR})
    #
    #    if(${OPENSSL})
    #        target_include_directories(pubnub-unit-test PUBLIC
    #            ${CMAKE_CURRENT_LIST_DIR}/openssl/
    #            ${CMAKE_CURRENT_LIST_DIR}/lib/base64)
    #        target_include_directories(pubnub-unit-test PUBLIC ${OPENSSL_INCLUDE_DIR})
    #    else()
    #        if(UNIX)
    #            target_include_directories(pubnub-unit-test PUBLIC ${CMAKE_CURRENT_LIST_DIR}/posix)
    #        elseif(WIN32 OR WIN64 OR MSVC)
    #            target_include_directories(pubnub-unit-test PUBLIC
    #                ${CMAKE_CURRENT_LIST_DIR}/windows
    #                ${CMAKE_CURRENT_LIST_DIR}/core/c99)
    #        endif()
    #    endif()
    #
    #    target_link_libraries(pubnub-unit-test ${LDLIBS} pubnub ${CGREEN_ROOT_DIR}/build/src/libcgreen.a)
    #
    #    add_custom_target(RunUnitTests
    #        COMMAND ${CGREEN_ROOT_DIR/build/tools/cgreen-runner} pubnub-unit-test)

    # TODO: finish unit tests
endif ()
    

