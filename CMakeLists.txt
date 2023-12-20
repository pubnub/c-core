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
num_option(USE_REVOKE_TOKEN_API "Use revoke token API" ON)
num_option(USE_GRANT_TOKEN_API "Use grant token API" ON)
num_option(USE_FETCH_HISTORY "Use fetch history" ON)

option(EXAMPLES "Build all examples" OFF)
option(UNIT_TEST "Build and run unit tests" OFF)

set(EXAMPLE_NAME "" CACHE STRING "Example name to build if EXAMPLES=OFF and want to build only one example")

option(CALLBACK_API "Use callback API [CALLBACK=ON SYNC=OFF]" OFF)
option(SHARED_LIB "Library type. [SHARED=ON STATIC=OFF]" OFF)

# Flags configuration

set(LDLIBS "-lpthread")

set(CMAKE_CXX_FLAGS 
    "-g -Wall -D PUBNUB_THREADSAFE -D PUBNUB_LOG_LEVEL=PUBNUB_LOG_LEVEL_WARNING -D PUBNUB_ONLY_PUBSUB_API=${ONLY_PUBSUB_API} -D PUBNUB_PROXY_API=${USE_PROXY} -D PUBNUB_USE_GZIP_COMPRESSION=${USE_GZIP_COMPRESSION} -D PUBNUB_RECEIVE_GZIP_RESPONSE=${RECEIVE_GZIP_RESPONSE} -D PUBNUB_USE_SUBSCRIBE_V2=${USE_SUBSCRIBE_V2} -D PUBNUB_USE_OBJECTS_API=${USE_OBJECTS_API} -D PUBNUB_USE_ACTIONS_API=${USE_ACTIONS_API} -D PUBNUB_USE_AUTO_HEARTBEAT=${USE_AUTO_HEARTBEAT} -D PUBNUB_USE_GRANT_TOKEN_API=${USE_GRANT_TOKEN_API} -D PUBNUB_USE_REVOKE_TOKEN_API=${USE_REVOKE_TOKEN_API} -D PUBNUB_USE_FETCH_HISTORY=${USE_FETCH_HISTORY} ${CMAKE_CXX_FLAGS}")

message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

set(CORE_SOURCEFILES ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_set_state.c ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_pubsubapi.c core/pubnub_coreapi.c ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_coreapi_ex.c ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_ccore_pubsub.c ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_ccore.c ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_netcore.c ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_alloc_std.c ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_assert_std.c ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_generate_uuid.c ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_blocking_io.c ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_timers.c ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_json_parse.c  ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_helper.c ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_generate_uuid_v3_md5.c  ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_free_with_timeout_std.c ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_url_encode.c ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_memory_block.c )

set(LIB_SOURCEFILES ${CMAKE_CURRENT_LIST_DIR}/lib/sockets/pbpal_sockets.c ${CMAKE_CURRENT_LIST_DIR}/lib/sockets/pbpal_resolv_and_connect_sockets.c ${CMAKE_CURRENT_LIST_DIR}/lib/sockets/pbpal_handle_socket_error.c ${CMAKE_CURRENT_LIST_DIR}/lib/md5/md5.c ${CMAKE_CURRENT_LIST_DIR}/lib/base64/pbbase64.c ${CMAKE_CURRENT_LIST_DIR}/lib/pb_strnlen_s.c ${CMAKE_CURRENT_LIST_DIR}/lib/pb_strncasecmp.c  )

set(POSIX_SOURCEFILES ${CMAKE_CURRENT_LIST_DIR}/posix/posix_socket_blocking_io.c  ${CMAKE_CURRENT_LIST_DIR}/posix/pubnub_version_posix.c ${CMAKE_CURRENT_LIST_DIR}/posix/pubnub_generate_uuid_posix.c ${CMAKE_CURRENT_LIST_DIR}/posix/pbpal_posix_blocking_io.c ${CMAKE_CURRENT_LIST_DIR}/posix/msstopwatch_monotonic_clock.c ${CMAKE_CURRENT_LIST_DIR}/posix/pbtimespec_elapsed_ms.c ${CMAKE_CURRENT_LIST_DIR}/posix/pb_sleep_ms.c)

# if MACOS
set(POSIX_SOURCEFILES ${POSIX_SOURCEFILES} ${CMAKE_CURRENT_LIST_DIR}/posix/monotonic_clock_get_time_darwin.c)
# else
#set(POSIX_SOURCEFILES ${POSIX_SOURCEFILES} ${CMAKE_CURRENT_LIST_DIR}/posix/monotonic_clock_get_time_posix.c)

set(INTF_SOURCEFILES)

if(${CALLBACK_API})
    message(STATUS "Using callback API")
    set(INTF_SOURCEFILES ${INTF_SOURCEFILES}  ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_timer_list.c ${CMAKE_CURRENT_LIST_DIR}/lib/sockets/pbpal_ntf_callback_poller_poll.c ${CMAKE_CURRENT_LIST_DIR}/lib/sockets/pbpal_adns_sockets.c ${CMAKE_CURRENT_LIST_DIR}/lib/pubnub_dns_codec.c ${CMAKE_CURRENT_LIST_DIR}/core/pbpal_ntf_callback_queue.c ${CMAKE_CURRENT_LIST_DIR}/core/pbpal_ntf_callback_admin.c ${CMAKE_CURRENT_LIST_DIR}/core/pbpal_ntf_callback_handle_timer_list.c  ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_callback_subscribe_loop.c)
    set(INTF_SOURCEFILES ${INTF_SOURCEFILES} ${CMAKE_CURRENT_LIST_DIR}/posix/pubnub_ntf_callback_posix.c ${CMAKE_CURRENT_LIST_DIR}/posix/pubnub_get_native_socket.c)
else()
    message(STATUS "Using sync API")
    set(INTF_SOURCEFILES ${INTF_SOURCEFILES} ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_ntf_sync.c ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_sync_subscribe_loop.c ${CMAKE_CURRENT_LIST_DIR}/core/srand_from_pubnub_time.c)
endif()

set(FEATURE_SOURCEFILES)

if(${USE_PROXY})
    set(FEATURE_SOURCEFILES ${FEATURE_SOURCEFILES} ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_proxy.c ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_proxy_core.c ${CMAKE_CURRENT_LIST_DIR}/core/pbhttp_digest.c ${CMAKE_CURRENT_LIST_DIR}/core/pbntlm_core.c ${CMAKE_CURRENT_LIST_DIR}/core/pbntlm_packer_std.c)
endif()

if(${USE_GZIP_COMPRESSION})
    set(FEATURE_SOURCEFILES ${FEATURE_SOURCEFILES} ${CMAKE_CURRENT_LIST_DIR}/lib/miniz/miniz_tdef.c ${CMAKE_CURRENT_LIST_DIR}/lib/miniz/miniz.c ${CMAKE_CURRENT_LIST_DIR}/lib/pbcrc32.c ${CMAKE_CURRENT_LIST_DIR}/core/pbgzip_compress.c)
endif()

if(${RECEIVE_GZIP_RESPONSE})
    set(FEATURE_SOURCEFILES ${FEATURE_SOURCEFILES} ${CMAKE_CURRENT_LIST_DIR}/lib/miniz/miniz_tinfl.c ${CMAKE_CURRENT_LIST_DIR}/core/pbgzip_decompress.c)
endif()

if(${USE_SUBSCRIBE_V2})
    set(FEATURE_SOURCEFILES ${FEATURE_SOURCEFILES} ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_subscribe_v2.c ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_subscribe_v2.c)
endif()

if(${USE_ADVANCED_HISTORY})
    set(FEATURE_SOURCEFILES ${FEATURE_SOURCEFILES} ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_advanced_history.c ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_advanced_history.c)
endif()

if(${USE_OBJECTS_API})
    set(FEATURE_SOURCEFILES ${FEATURE_SOURCEFILES} ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_objects_api.c ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_objects_api.c)
endif()

if(${USE_ACTIONS_API})
    set(FEATURE_SOURCEFILES ${FEATURE_SOURCEFILES} ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_actions_api.c ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_actions_api.c)
endif()

if(${USE_AUTO_HEARTBEAT})
    set(FEATURE_SOURCEFILES ${FEATURE_SOURCEFILES} ${CMAKE_CURRENT_LIST_DIR}/core/pbauto_heartbeat.c ${CMAKE_CURRENT_LIST_DIR}/posix/pbauto_heartbeat_init_posix.c ${CMAKE_CURRENT_LIST_DIR}/lib/pbstr_remove_from_list.c)
endif()

if(${USE_FETCH_HISTORY})
    set(FEATURE_SOURCEFILES ${FEATURE_SOURCEFILES} ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_fetch_history.c ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_fetch_history.c)
endif()

if(${USE_GRANT_TOKEN_API})
    set(FEATURE_SOURCEFILES ${FEATURE_SOURCEFILES} ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_grant_token_api.c ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_grant_token_api.c)
endif()

if(${USE_REVOKE_TOKEN_API})
    set(FEATURE_SOURCEFILES ${FEATURE_SOURCEFILES} ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_revoke_token_api.c ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_revoke_token_api.c)
endif()

set(SOURCEFILES ${CORE_SOURCEFILES} ${LIB_SOURCEFILES} ${POSIX_SOURCEFILES} ${FEATURE_SOURCEFILES} ${INTF_SOURCEFILES})

include_directories(${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/core ${CMAKE_CURRENT_LIST_DIR}/lib ${CMAKE_CURRENT_LIST_DIR}/posix)

# original
#set(SOURCEFILES "core/pbcc_set_state.c core/pubnub_pubsubapi.c core/pubnub_coreapi.c ../core/pubnub_coreapi_ex.c ../core/pubnub_ccore_pubsub.c ../core/pubnub_ccore.c ../core/pubnub_netcore.c  ../lib/sockets/pbpal_sockets.c ../lib/sockets/pbpal_resolv_and_connect_sockets.c ../lib/sockets/pbpal_handle_socket_error.c ../core/pubnub_alloc_std.c ../core/pubnub_assert_std.c ../core/pubnub_generate_uuid.c ../core/pubnub_blocking_io.c ../posix/posix_socket_blocking_io.c ../core/pubnub_timers.c ../core/pubnub_json_parse.c  ../lib/md5/md5.c ../lib/base64/pbbase64.c ../lib/pb_strnlen_s.c ../lib/pb_strncasecmp.c ../core/pubnub_helper.c pubnub_version_posix.c pubnub_generate_uuid_posix.c pbpal_posix_blocking_io.c ../core/pubnub_generate_uuid_v3_md5.c  ../core/pubnub_free_with_timeout_std.c msstopwatch_monotonic_clock.c pbtimespec_elapsed_ms.c ../core/pubnub_url_encode.c ../core/pubnub_memory_block.c ../posix/pb_sleep_ms.c")

#add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/core)

if(${SHARED_LIB})
    message(STATUS "Building shared library")
    set(LIBTYPE SHARED)
else()
    message(STATUS "Building static library")
    set(LIBYTPE STATIC)
endif()

add_library(pubnub ${LIBTYPE} ${SOURCEFILES})

target_link_libraries(pubnub ${LDLIBS})

add_executable(pubnub_sync_sample ${CMAKE_CURRENT_LIST_DIR}/core/samples/pubnub_sync_sample.c)

target_link_libraries(pubnub_sync_sample pubnub ${LDLIBS})
