# Makefile for PubNub C-core SDK.
# Description: All-in-one make file which show how to build various versions of
#              PubNub SDK which has specific features and link them with application.

USE_CALLBACK_API = 1

include ../make/posix_preprocessing.mk
include ../make/posix_source_files.mk
include ../make/posix_compiler_linker_flags.mk


###############################################################################
#                                Build targets                                #
###############################################################################

include ../make/common/targets_lib.mk
include ../make/posix_targets.mk

# There is also `subscribe_event_engine_sample$(APP_EXT)` target but it can't be built
# with `all` because requires `USE_SUBSCRIBE_EVENT_ENGINE=1`.

all: \
	cancel_subscribe_sync_sample$(APP_EXT)      \
	metadata$(APP_EXT)                          \
	pubnub_advanced_history_sample$(APP_EXT)    \
	pubnub_fetch_history_sample$(APP_EXT)       \
	pubnub_publish_via_post_sample$(APP_EXT)    \
	pubnub_sync_publish_retry$(APP_EXT)         \
	pubnub_sync_sample$(APP_EXT)                \
	pubnub_sync_secretkey_sample$(APP_EXT)      \
	pubnub_sync_subloop_sample$(APP_EXT)        \
	publish_callback_subloop_sample$(APP_EXT)   \
	publish_queue_callback_subloop$(APP_EXT)    \
	pubnub_callback_sample$(APP_EXT)            \
	pubnub_callback_subloop_sample$(APP_EXT)    \
	subscribe_publish_callback_sample$(APP_EXT) \
	subscribe_publish_from_callback$(APP_EXT)   \
	pubnub_console_sync$(APP_EXT)               \
	pubnub_console_callback$(APP_EXT)           \
	pubnub_fntest$(APP_EXT)

clean:
	find . -type d -iname "*.dSYM" -exec rm -rf {} \+
	find . -type f ! -name "*.*" -o -name "*.a" -o -name "*.o" | xargs -r rm -rf -rf