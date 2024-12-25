# Makefile for PubNub C-core SDK with SSL support.
# Description: All-in-one make file which show how to build various versions of
#              PubNub SDK which has specific features and link them with application.

# Making CPP version of the PubNub SDK with OpenSSL support.
WITH_CPP = 1
OPENSSL = 1
USE_CRYPTO_API ?= 1
USE_GRANT_TOKEN ?= 1
USE_REVOKE_TOKEN ?= 1

include ../make/posix_preprocessing.mk
include ../make/posix_source_files.mk
include ../make/posix_compiler_linker_flags.mk


###############################################################################
#                                Build targets                                #
###############################################################################

TARGET_BUILD_PATH = openssl$(PATH_SEP)

include ../make/windows_cpp_targets.mk


all: \
    $(TARGET_BUILD_PATH)pubnub_callback_cpp11_sample$(APP_EXT)         \
    $(TARGET_BUILD_PATH)pubnub_callback_sample$(APP_EXT)               \
    $(TARGET_BUILD_PATH)pubnub_crypto_module_sample$(APP_EXT)          \
    $(TARGET_BUILD_PATH)subscribe_publish_callback_sample$(APP_EXT)    \
    $(TARGET_BUILD_PATH)pubnub_sync_grant_sample$(APP_EXT)             \
    $(TARGET_BUILD_PATH)pubnub_sync_sample$(APP_EXT)                   \
    $(TARGET_BUILD_PATH)pubnub_sync_revoke_sample$(APP_EXT)            \
    $(TARGET_BUILD_PATH)cancel_subscribe_sync_sample$(APP_EXT)         \
    $(TARGET_BUILD_PATH)futres_nesting_sync$(APP_EXT)                  \
    $(TARGET_BUILD_PATH)futres_nesting_callback$(APP_EXT)              \
    $(TARGET_BUILD_PATH)futres_nesting_callback_cpp11$(APP_EXT)        \
    $(TARGET_BUILD_PATH)fntest_runner$(APP_EXT)

clean:
	del $(TARGET_BUILD_PATH)*.obj
	del $(TARGET_BUILD_PATH)*.exe
	del $(TARGET_BUILD_PATH)*.pdb
	del $(TARGET_BUILD_PATH)*.il?