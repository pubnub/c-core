cmake_minimum_required(VERSION 3.12)

project(c-core VERSION 4.8.0)

option(ONLY_PUBSUB_API "Only pubsub API" OFF)
option(USE_PROXY "Use proxy" ON)
option(USE_GZIP_COMPRESSION "Use gzip compression" ON)
option(RECEIVE_GZIP_RESPONSE "Receive gzip response" ON)
option(USE_SUBSCRIBE_V2 "Use subscribe v2" ON)
option(USE_ADVANCED_HISTORY "Use advanced history" ON)
option(USE_OBJECTS_API "Use objects API" ON)
option(USE_AUTO_HEARTBEAT "Use auto heartbeat" ON)
option(USE_ACTIONS_API "Use actions API" ON)
option(USE_REVOKE_TOKEN_API "Use revoke token API" OFF)
option(USE_GRANT_TOKEN_API "Use grant token API" OFF)
option(USE_FETCH_HISTORY "Use fetch history" ON)

set(CMAKE_CXX_FLAGS 
    "-g -Wall -D PUBNUB_THREADSAFE -D PUBNUB_LOG_LEVEL=PUBNUB_LOG_LEVEL_WARNING -D PUBNUB_ONLY_PUBSUB_API=${ONLY_PUBSUB_API} -D PUBNUB_PROXY_API=${USE_PROXY} -D PUBNUB_USE_GZIP_COMPRESSION=${USE_GZIP_COMPRESSION} -D PUBNUB_RECEIVE_GZIP_RESPONSE=${RECEIVE_GZIP_RESPONSE} -D PUBNUB_USE_SUBSCRIBE_V2=${USE_SUBSCRIBE_V2} -D PUBNUB_USE_OBJECTS_API=${USE_OBJECTS_API} -D PUBNUB_USE_ACTIONS_API=${USE_ACTIONS_API} -D PUBNUB_USE_AUTO_HEARTBEAT=${USE_AUTO_HEARTBEAT} -D PUBNUB_USE_GRANT_TOKEN_API=${USE_GRANT_TOKEN} -D PUBNUB_USE_REVOKE_TOKEN_API=${USE_REVOKE_TOKEN} -D PUBNUB_USE_FETCH_HISTORY=${USE_FETCH_HISTORY} ${CMAKE_CXX_FLAGS}")

option(SHARED_LIB "Library type. [SHARED=ON STATIC=OFF]" OFF)


if(${SHARED_LIB})
    set(LIBTYPE SHARED)
else()
    set(LIBYTPE STATIC)
endif()

set(CORE_SOURCEFILES ${CMAKE_CURRENT_LIST_DIR}/core/pbcc_set_state.c ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_pubsubapi.c core/pubnub_coreapi.c ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_coreapi_ex.c ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_ccore_pubsub.c ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_ccore.c ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_netcore.c ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_alloc_std.c ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_assert_std.c ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_generate_uuid.c ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_blocking_io.c ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_timers.c ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_json_parse.c  ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_helper.c ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_generate_uuid_v3_md5.c  ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_free_with_timeout_std.c ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_url_encode.c ${CMAKE_CURRENT_LIST_DIR}/core/pubnub_memory_block.c )

set(LIB_SOURCEFILES${CMAKE_CURRENT_LIST_DIR}/lib/sockets/pbpal_sockets.c ${CMAKE_CURRENT_LIST_DIR}/lib/sockets/pbpal_resolv_and_connect_sockets.c ${CMAKE_CURRENT_LIST_DIR}/lib/sockets/pbpal_handle_socket_error.c ${CMAKE_CURRENT_LIST_DIR}/lib/md5/md5.c ${CMAKE_CURRENT_LIST_DIR}/lib/base64/pbbase64.c ${CMAKE_CURRENT_LIST_DIR}/lib/pb_strnlen_s.c ${CMAKE_CURRENT_LIST_DIR}/lib/pb_strncasecmp.c  )

set(POSIX_SOURCEFILES ${CMAKE_CURRENT_LIST_DIR}/posix/posix_socket_blocking_io.c  ${CMAKE_CURRENT_LIST_DIR}/posix/pubnub_version_posix.c ${CMAKE_CURRENT_LIST_DIR}/posix/pubnub_generate_uuid_posix.c ${CMAKE_CURRENT_LIST_DIR}/posix/pbpal_posix_blocking_io.c ${CMAKE_CURRENT_LIST_DIR}/posix/msstopwatch_monotonic_clock.c ${CMAKE_CURRENT_LIST_DIR}/posix/pbtimespec_elapsed_ms.c ${CMAKE_CURRENT_LIST_DIR}/posix/pb_sleep_ms.c)

set(SOURCEFILES ${CORE_SOURCEFILES} ${LIB_SOURCEFILES} ${POSIX_SOURCEFILES})

include_directories(${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/core ${CMAKE_CURRENT_LIST_DIR}/lib ${CMAKE_CURRENT_LIST_DIR}/posix)

# original
#set(SOURCEFILES "core/pbcc_set_state.c core/pubnub_pubsubapi.c core/pubnub_coreapi.c ../core/pubnub_coreapi_ex.c ../core/pubnub_ccore_pubsub.c ../core/pubnub_ccore.c ../core/pubnub_netcore.c  ../lib/sockets/pbpal_sockets.c ../lib/sockets/pbpal_resolv_and_connect_sockets.c ../lib/sockets/pbpal_handle_socket_error.c ../core/pubnub_alloc_std.c ../core/pubnub_assert_std.c ../core/pubnub_generate_uuid.c ../core/pubnub_blocking_io.c ../posix/posix_socket_blocking_io.c ../core/pubnub_timers.c ../core/pubnub_json_parse.c  ../lib/md5/md5.c ../lib/base64/pbbase64.c ../lib/pb_strnlen_s.c ../lib/pb_strncasecmp.c ../core/pubnub_helper.c pubnub_version_posix.c pubnub_generate_uuid_posix.c pbpal_posix_blocking_io.c ../core/pubnub_generate_uuid_v3_md5.c  ../core/pubnub_free_with_timeout_std.c msstopwatch_monotonic_clock.c pbtimespec_elapsed_ms.c ../core/pubnub_url_encode.c ../core/pubnub_memory_block.c ../posix/pb_sleep_ms.c")

#add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/core)

add_library(pubnub ${LIBTYPE} ${SOURCEFILES})

target_link_libraries(pubnub)
