# CMakeLists.txt for the C-core library of Pubnub.
cmake_minimum_required(VERSION 3.12)

project(c-core VERSION 4.8.0)

message(STATUS Preparing\ ${PROJECT_NAME}\ version\ ${PROJECT_VERSION})

# Function to convert between "ON"/"OFF" and 1/0
function(num_option name description default)
    option(${name} ${description} ${default})
    if(${name})
        set(${name} 1 PARENT_SCOPE)
    else()
        set(${name} 0 PARENT_SCOPE)
    endif()
endfunction()

# All the options that can be set on the command line to control

num_option(ONLY_PUBSUB_API "Only pubsub API" OFF)
num_option(USE_PROXY "Use proxy" ON)
num_option(USE_GZIP_COMPRESSION "Use gzip compression" ON)
num_option(RECEIVE_GZIP_RESPONSE "Use gzip decompression" ON)
num_option(USE_SUBSCRIBE_V2 "Use subscribe v2" ON)
num_option(USE_ADVANCED_HISTORY "Use advanced history" ON)
num_option(USE_OBJECTS_API "Use objects API" ON)
num_option(USE_AUTO_HEARTBEAT "Use auto heartbeat" ON)
num_option(USE_ACTIONS_API "Use actions API" ON)
num_option(USE_REVOKE_TOKEN_API "Use revoke token API" OFF)
num_option(USE_GRANT_TOKEN_API "Use grant token API" OFF)
num_option(USE_FETCH_HISTORY "Use fetch history" ON)
num_option(USE_CRYPTO_API "Use crypto API [OPENSSL ONLY]" OFF)

option(OPENSSL "Use OpenSSL" ON)
set(OPENSSL_ROOT_DIR "" CACHE STRING "OpenSSL root directory")
option(EXAMPLES "Build all examples" OFF)
set(EXAMPLE "all" CACHE STRING "Build example with provided name (use 'all' for all examples) [EXAMPLES=ON needed]")
option(UNIT_TEST "Build and run unit tests" OFF)
option(CALLBACK_API "Use callback API [CALLBACK=ON SYNC=OFF]" OFF)
option(SHARED_LIB "Library type. [SHARED=ON STATIC=OFF]" OFF)

# Flags configuration


set(CMAKE_C_FLAGS "\
    -g \
    -Wall \
    -D PUBNUB_THREADSAFE \
    -D PUBNUB_LOG_LEVEL=PUBNUB_LOG_LEVEL_WARNING \
    -D PUBNUB_ONLY_PUBSUB_API=${ONLY_PUBSUB_API} \
    -D PUBNUB_PROXY_API=${USE_PROXY} \
    -D PUBNUB_USE_GZIP_COMPRESSION=${USE_GZIP_COMPRESSION} \
    -D PUBNUB_RECEIVE_GZIP_RESPONSE=${RECEIVE_GZIP_RESPONSE} \
    -D PUBNUB_USE_SUBSCRIBE_V2=${USE_SUBSCRIBE_V2} \
    -D PUBNUB_USE_OBJECTS_API=${USE_OBJECTS_API} \
    -D PUBNUB_USE_ACTIONS_API=${USE_ACTIONS_API} \
    -D PUBNUB_USE_AUTO_HEARTBEAT=${USE_AUTO_HEARTBEAT} \
    -D PUBNUB_USE_GRANT_TOKEN_API=${USE_GRANT_TOKEN_API} \
    -D PUBNUB_USE_REVOKE_TOKEN_API=${USE_REVOKE_TOKEN_API} \
    -D PUBNUB_USE_FETCH_HISTORY=${USE_FETCH_HISTORY} \
    ${CMAKE_C_FLAGS}")

message(STATUS "CMAKE_C_FLAGS: ${CMAKE_C_FLAGS}")

set(CORE_SOURCEFILES 
    ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_set_state.c
    ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_pubsubapi.c
    ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_coreapi.c
    ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_coreapi_ex.c
    ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_ccore_pubsub.c
    ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_ccore.c
    ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_netcore.c
    ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_alloc_std.c
    ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_assert_std.c
    ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_generate_uuid.c
    ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_blocking_io.c
    ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_timers.c
    ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_json_parse.c
    ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_helper.c
    ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_generate_uuid_v3_md5.c
    ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_free_with_timeout_std.c
    ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_url_encode.c
    ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_memory_block.c) 

set(LIB_SOURCEFILES
    ${CMAKE_CURRENT_LIST_DIR}/lib/sockets/pbpal_resolv_and_connect_sockets.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/sockets/pbpal_handle_socket_error.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/md5/md5.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/base64/pbbase64.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/pb_strnlen_s.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/pb_strncasecmp.c)

set(LDLIBS)
set(OS_SOURCEFILES)

if(UNIX)
    set(LDLIBS "-lpthread")
    set(OS_SOURCEFILES
        ${CMAKE_CURRENT_LIST_DIR}/posix/posix_socket_blocking_io.c
        ${CMAKE_CURRENT_LIST_DIR}/posix/pubnub_version_posix.c
        ${CMAKE_CURRENT_LIST_DIR}/posix/pubnub_generate_uuid_posix.c
        ${CMAKE_CURRENT_LIST_DIR}/posix/msstopwatch_monotonic_clock.c
        ${CMAKE_CURRENT_LIST_DIR}/posix/pbtimespec_elapsed_ms.c
        ${CMAKE_CURRENT_LIST_DIR}/posix/pb_sleep_ms.c)

    if(APPLE)
        set(OS_SOURCEFILES ${OS_SOURCEFILES} ${CMAKE_CURRENT_LIST_DIR}/posix/monotonic_clock_get_time_darwin.c)
    else()
        set(OS_SOURCEFILES ${OS_SOURCEFILES} ${CMAKE_CURRENT_LIST_DIR}/posix/monotonic_clock_get_time_posix.c)
    endif()
elseif(WIN32 || WIN64 || MSVC)
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
endif()

set(INTF_SOURCEFILES)

if(${CALLBACK_API})
    message(STATUS "Using callback API")

    set(INTF_SOURCEFILES
        ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_timer_list.c
        ${CMAKE_CURRENT_LIST_DIR}/lib/sockets/pbpal_ntf_callback_poller_poll.c
        ${CMAKE_CURRENT_LIST_DIR}/lib/sockets/pbpal_adns_sockets.c
        ${CMAKE_CURRENT_LIST_DIR}/lib/pubnub_dns_codec.c
        ${CMAKE_CURRENT_LIST_DIR}/core/pbpal_ntf_callback_queue.c
        ${CMAKE_CURRENT_LIST_DIR}/core/pbpal_ntf_callback_admin.c
        ${CMAKE_CURRENT_LIST_DIR}/core/pbpal_ntf_callback_handle_timer_list.c
        ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_callback_subscribe_loop.c
        ${INTF_SOURCEFILES})

    set(INTF_SOURCEFILES
        ${INTF_SOURCEFILES}
        ${CMAKE_CURRENT_LIST_DIR}/posix/pubnub_ntf_callback_posix.c
        ${CMAKE_CURRENT_LIST_DIR}/posix/pubnub_get_native_socket.c)
else()
    message(STATUS "Using sync API")

    set(INTF_SOURCEFILES
        ${INTF_SOURCEFILES}
        ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_ntf_sync.c
        ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_sync_subscribe_loop.c
        ${CMAKE_CURRENT_LIST_DIR}/core/srand_from_pubnub_time.c)
endif()

set(FEATURE_SOURCEFILES)

if(${USE_PROXY})
    set(FEATURE_SOURCEFILES
        ${FEATURE_SOURCEFILES}
        ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_proxy.c
        ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_proxy_core.c
        ${CMAKE_CURRENT_LIST_DIR}/core/pbhttp_digest.c
        ${CMAKE_CURRENT_LIST_DIR}/core/pbntlm_core.c
        ${CMAKE_CURRENT_LIST_DIR}/core/pbntlm_packer_std.c)
endif()

if(${USE_GZIP_COMPRESSION})
    set(FEATURE_SOURCEFILES
        ${FEATURE_SOURCEFILES}
        ${CMAKE_CURRENT_LIST_DIR}/lib/miniz/miniz_tdef.c
        ${CMAKE_CURRENT_LIST_DIR}/lib/miniz/miniz.c
        ${CMAKE_CURRENT_LIST_DIR}/lib/pbcrc32.c
        ${CMAKE_CURRENT_LIST_DIR}/core/pbgzip_compress.c)
endif()

if(${RECEIVE_GZIP_RESPONSE})
    set(FEATURE_SOURCEFILES
        ${FEATURE_SOURCEFILES}
        ${CMAKE_CURRENT_LIST_DIR}/lib/miniz/miniz_tinfl.c
        ${CMAKE_CURRENT_LIST_DIR}/core/pbgzip_decompress.c)
endif()

if(${USE_SUBSCRIBE_V2})
    set(FEATURE_SOURCEFILES
        ${FEATURE_SOURCEFILES}
        ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_subscribe_v2.c
        ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_subscribe_v2.c)
endif()

if(${USE_ADVANCED_HISTORY})
    set(FEATURE_SOURCEFILES
        ${FEATURE_SOURCEFILES}
        ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_advanced_history.c
        ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_advanced_history.c)
endif()

if(${USE_OBJECTS_API})
    set(FEATURE_SOURCEFILES 
        ${FEATURE_SOURCEFILES} 
        ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_objects_api.c 
        ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_objects_api.c)
endif()

if(${USE_ACTIONS_API})
    set(FEATURE_SOURCEFILES 
        ${FEATURE_SOURCEFILES} 
        ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_actions_api.c 
        ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_actions_api.c)
endif()

if(${USE_AUTO_HEARTBEAT})
    set(FEATURE_SOURCEFILES 
        ${FEATURE_SOURCEFILES} 
        ${CMAKE_CURRENT_LIST_DIR}/core/pbauto_heartbeat.c 
        ${CMAKE_CURRENT_LIST_DIR}/lib/pbstr_remove_from_list.c)
    if(NOT ${OPENSSL})
        set(FEATURE_SOURCEFILES 
            ${FEATURE_SOURCEFILES} 
            ${CMAKE_CURRENT_LIST_DIR}/core/pbauto_heartbeat_init_posix.c)
    endif()
endif()

if(${USE_FETCH_HISTORY})
    set(FEATURE_SOURCEFILES 
        ${FEATURE_SOURCEFILES} 
        ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_fetch_history.c 
        ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_fetch_history.c)
endif()

if(${USE_GRANT_TOKEN_API})
    set(FEATURE_SOURCEFILES 
        ${FEATURE_SOURCEFILES} 
        ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_grant_token_api.c 
        ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_grant_token_api.c)
endif()

if(${USE_REVOKE_TOKEN_API})
    set(FEATURE_SOURCEFILES 
        ${FEATURE_SOURCEFILES} 
        ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_revoke_token_api.c 
        ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_revoke_token_api.c)
endif()

set(SOURCEFILES 
    ${CORE_SOURCEFILES} 
    ${LIB_SOURCEFILES} 
    ${OS_SOURCEFILES} 
    ${FEATURE_SOURCEFILES} 
    ${INTF_SOURCEFILES})

add_library(pubnub ${LIBTYPE} ${SOURCEFILES})

set(OPENSSL_LINK_LIBRARIES)
if(${OPENSSL})
    message(STATUS "Using OpenSSL")

    if("${OPENSSL_ROOT_DIR}" STREQUAL "")
        find_package(OpenSSL REQUIRED)
    else()
        find_package(OpenSSL REQUIRED PATHS ${OPENSSL_ROOT_DIR})
    endif()

    set(FEATURE_SOURCEFILES 
        ${OPENSSL_LIBRARIES} 
        ${FEATURE_SOURCEFILES})
    target_include_directories(pubnub PUBLIC ${OPENSSL_INCLUDE_DIR})

    set(LIB_SOURCEFILES 
        ${CMAKE_CURRENT_LIST_DIR}/lib/base64/pbbase64.c 
        ${LIB_SOURCEFILES})

    set(FEATURE_SOURCEFILES 
        ${CMAKE_CURRENT_LIST_DIR}/openssl/pbpal_openssl.c 
        ${CMAKE_CURRENT_LIST_DIR}/openssl/pbpal_connect_openssl.c 
        ${CMAKE_CURRENT_LIST_DIR}/openssl/pbpal_add_system_certs_posix.c  
        ${CMAKE_CURRENT_LIST_DIR}/openssl/pbpal_openssl_blocking_io.c 
        ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_crypto.c 
        ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_crypto_aes_cbc.c 
        ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_crypto_legacy.c 
        ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_crypto.c 
        ${CMAKE_CURRENT_LIST_DIR}/openssl/pbaes256.c 
        ${FEATURE_SOURCEFILES})

    set(FEATURE_SOURCEFILES 
        ${CMAKE_CURRENT_LIST_DIR}/openssl/pbauto_heartbeat_init_posix.c 
        ${FEATURE_SOURCEFILES})

    set(LDLIBS  ${LDLIBS})
    set(OPENSSL_LINK_LIBRARIES OpenSSL::SSL OpenSSL::Crypto)
endif()

target_include_directories(pubnub PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/cpp
    ${CMAKE_CURRENT_LIST_DIR}/core 
    ${CMAKE_CURRENT_LIST_DIR}/lib)

if(${OPENSSL})
    target_include_directories(pubnub PUBLIC 
        ${CMAKE_CURRENT_LIST_DIR}/openssl/
        ${CMAKE_CURRENT_LIST_DIR}/lib/base64)
else()
    set(LIB_SOURCEFILES 
        ${CMAKE_CURRENT_LIST_DIR}/lib/sockets/pbpal_sockets.c 
        ${LIB_SOURCEFILES})
    if(UNIX)
        set(LIB_SOURCEFILES 
            ${CMAKE_CURRENT_LIST_DIR}/posix/pbpal_posix_blocking_io.c 
            ${LIB_SOURCEFILES})
        target_include_directories(pubnub PUBLIC ${CMAKE_CURRENT_LIST_DIR}/posix)
    elseif(WIN32 || WIN64 || MSVC)
        set(LIB_SOURCEFILES 
            ${CMAKE_CURRENT_LIST_DIR}/windows/pbpal_windows_blocking_io.c 
            ${LIB_SOURCEFILES})
        target_include_directories(pubnub PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}/windows
            ${CMAKE_CURRENT_LIST_DIR}/core/c99)
    endif()
endif()

if(${SHARED_LIB})
    message(STATUS "Building shared library")
    set(LIBTYPE SHARED)
else()
    message(STATUS "Building static library")
    set(LIBYTPE STATIC)
endif()

target_link_libraries(pubnub ${LDLIBS} ${OPENSSL_LINK_LIBRARIES})

if(${EXAMPLES})
    message(STATUS "Building all examples")
    set(EXAMPLE_LIST)

    if(${EXAMPLE} STREQUAL "all")
        if(${CALLBACK_API})
            message(STATUS "Building callback examples")
            set(EXAMPLE_LIST
                pubnub_callback_sample
                pubnub_callback_subloop_sample
                subscribe_publish_callback_sample
                subscribe_publish_from_callback
                publish_callback_subloop_sample
                publish_queue_callback_subloop)
        else()
            message(STATUS "Building sync examples")
            set(EXAMPLE_LIST
                metadata
                pubnub_sync_subloop_sample
                pubnub_sync_publish_retry
                pubnub_publish_via_post_sample
                pubnub_advanced_history_sample
                pubnub_fetch_history_sample
                cancel_subscribe_sync_sample)
        endif()
    else()
        message(STATUS "Building example ${EXAMPLE}")
        set(EXAMPLE_LIST ${EXAMPLE})
    endif()

    foreach(EXAMPLE ${EXAMPLE_LIST})
        add_executable(${EXAMPLE} ${CMAKE_CURRENT_LIST_DIR}/core/samples/${EXAMPLE}.c)
        target_link_libraries(${EXAMPLE} pubnub ${LDLIBS})
    endforeach()
endif()

if(${UNIT_TEST})
    message(STATUS "Building unit tests")

    set(CMAKE_C_FLAGS " \
        ${CMAKE_C_FLAGS} \
        -D PUBNUB_ADVANCED_KEEP_ALIVE=1 \
        -D PUBNUB_DYNAMIC_REPLY_BUFFER=1") 

    # TODO: finish unit tests
endif()
    

