# Makefile for PubNub C-core SDK.
# Description: All-in-one make file which show how to build various versions of
#              PubNub SDK which has specific features and link them with application.

# Making CPP version of the PubNub SDK.
WITH_CPP = 1

include ../make/posix_preprocessing.mk
include ../make/posix_source_files.mk
include ../make/posix_compiler_linker_flags.mk


###############################################################################
#                                Build targets                                #
###############################################################################

TARGET_BUILD_PATH =

include ../make/posix_cpp_targets.mk


all: \
    $(TARGET_BUILD_PATH)pubnub_callback_cpp11_sample$(APP_EXT)         \
    $(TARGET_BUILD_PATH)pubnub_callback_sample$(APP_EXT)               \
    $(TARGET_BUILD_PATH)pubnub_callback_cpp11_subloop_sample$(APP_EXT) \
    $(TARGET_BUILD_PATH)pubnub_callback_subloop_sample$(APP_EXT)       \
    $(TARGET_BUILD_PATH)subscribe_publish_callback_sample$(APP_EXT)    \
    $(TARGET_BUILD_PATH)pubnub_sync_sample$(APP_EXT)                   \
    $(TARGET_BUILD_PATH)pubnub_sync_subloop_sample$(APP_EXT)           \
    $(TARGET_BUILD_PATH)cancel_subscribe_sync_sample$(APP_EXT)         \
    $(TARGET_BUILD_PATH)futres_nesting_sync$(APP_EXT)                  \
    $(TARGET_BUILD_PATH)futres_nesting_callback$(APP_EXT)              \
    $(TARGET_BUILD_PATH)futres_nesting_callback_cpp11$(APP_EXT)        \
    $(TARGET_BUILD_PATH)fntest_runner$(APP_EXT)

clean:
	find . -type d -iname "*.dSYM" -exec rm -rf {} \+
	find . -type f ! -name "*.*" -o -name "*.a" -o -name "*.o" | xargs -r rm -rf