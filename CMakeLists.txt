cmake_minimum_required(VERSION 3.5.1)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_STANDARD 14)
project (c-core)

#add_definitions(-DPUBNUB_THREADSAFE)

set(TARGET_NAME pubnub)

option(USE_SSL "Build with SSL support" ON)
if (USE_SSL)
    find_package(OpenSSL REQUIRED)
    set(SOURCES
            core/pubnub_ssl.c
            core/pubnub_pubsubapi.c
            core/pubnub_coreapi.c
            core/pubnub_ccore_pubsub.c
            core/pubnub_ccore.c
            core/pubnub_netcore.c
            openssl/pbpal_openssl.c
            openssl/pbpal_resolv_and_connect_openssl.c
            openssl/pbpal_add_system_certs_posix.c
            core/pubnub_alloc_std.c
            core/pubnub_assert_std.c
            core/pubnub_generate_uuid.c
            core/pubnub_blocking_io.c
            core/pubnub_timers.c
            core/pubnub_json_parse.c
            core/pubnub_helper.c
            openssl/pubnub_version_openssl.c
            posix/pubnub_generate_uuid_posix.c
            openssl/pbpal_openssl_blocking_io.c
            lib/base64/pbbase64.c
            core/pubnub_crypto.c
            core/pubnub_coreapi_ex.c
            core/pubnub_free_with_timeout_std.c
            openssl/pbaes256.c
            cpp/pubnub_subloop.cpp
            )
    set(PUBNUB_IMPLEMENTATION_DIR openssl)
else()
    set(SOURCES
            core/pubnub_pubsubapi.c
            core/pubnub_coreapi.c
            core/pubnub_coreapi_ex.c
            core/pubnub_ccore_pubsub.c
            core/pubnub_ccore.c
            core/pubnub_netcore.c
            lib/sockets/pbpal_sockets.c
            lib/sockets/pbpal_resolv_and_connect_sockets.c
            core/pubnub_alloc_std.c
            core/pubnub_assert_std.c
            core/pubnub_generate_uuid.c
            core/pubnub_blocking_io.c
            core/pubnub_timers.c
            core/pubnub_json_parse.c
            lib/md5/md5.c
            lib/base64/pbbase64.c
            core/pubnub_helper.c
            posix/pubnub_version_posix.c
            posix/pubnub_generate_uuid_posix.c
            posix/pbpal_posix_blocking_io.c
            core/pubnub_free_with_timeout_std.c
            cpp/pubnub_subloop.cpp
            )
    set(PUBNUB_IMPLEMENTATION_DIR posix)
endif()

#debug that
add_definitions(-DPUBNUB_LOG_LEVEL=PUBNUB_LOG_LEVEL_NONE)
#add_definitions(-DPUBNUB_LOG_LEVEL=PUBNUB_LOG_LEVEL_DEBUG)

option(USE_SYNC "Build with the sync thread model" ON)
if (USE_SYNC)
    list(APPEND SOURCES
            core/pubnub_ntf_sync.c
            cpp/pubnub_futres_sync.cpp
            )
else()
    add_definitions(-DPUBNUB_CALLBACK_API)
    list(APPEND SOURCES
            core/pubnub_timer_list.c
            ${PUBNUB_IMPLEMENTATION_DIR}/pubnub_ntf_callback_posix.c
            ${PUBNUB_IMPLEMENTATION_DIR}/pubnub_get_native_socket.c
            cpp/pubnub_futres_posix.cpp
            )
endif()

#proxy
list(APPEND SOURCES core/pubnub_proxy.c core/pubnub_proxy_core.c core/pbhttp_digest.c core/pbntlm_core.c core/pbntlm_packer_std.c)

#time
list(APPEND SOURCES posix/monotonic_clock_get_time_posix.c)

# Add the static library target
add_library(${TARGET_NAME} STATIC ${SOURCES})

# Include the source files
target_include_directories(${TARGET_NAME} PUBLIC core ${PUBNUB_IMPLEMENTATION_DIR} cpp lib/base64 lib/md5)

if (UNIX)
    # Install the resulting library into /usr/lib (by default)
    file(GLOB headers "cpp/pubnub*.hpp" "core/pubnub_*.h" "openssl/pubnub_*.h")
    install (FILES ${headers} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/)
    install (TARGETS ${TARGET_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
endif()
