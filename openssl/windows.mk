# Makefile for PubNub C-core SDK with SSL support.
# Description: All-in-one make file which show how to build various versions of
#              PubNub SDK which has specific features and link them with application.

# Making OpenSSL version of the PubNub SDK.
OPENSSL = 1
!ifndef USE_CRYPTO_API
USE_CRYPTO_API = 0
!endif
!ifndef USE_GRANT_TOKEN
USE_GRANT_TOKEN = 1
!endif
!ifndef USE_REVOKE_TOKEN
USE_REVOKE_TOKEN = 1
!endif

!include <../make/windows_preprocessing.mk>
!include <../make/windows_source_files.mk>
!include <../make/windows_compiler_linker_flags.mk>


###############################################################################
#                                Build targets                                #
###############################################################################

!include <../make/common/targets_lib.mk>
!include <../make/common/targets_app_openssl.mk>
!include <../make/windows_targets.mk>

# There is also `subscribe_event_engine_sample$(APP_EXT)` target but it can't be built
# with `all` because requires `USE_SUBSCRIBE_EVENT_ENGINE=1`.

all: \
	cancel_subscribe_sync_sample$(APP_EXT)                \
	metadata$(APP_EXT)                                    \
	pubnub_advanced_history_sample$(APP_EXT)              \
	pubnub_crypto_module_sample$(APP_EXT)                 \
	pubnub_crypto_sync_sample$(APP_EXT)                   \
	pubnub_encrypt_decrypt_dynamic_iv_sample$(APP_EXT)    \
	pubnub_encrypt_decrypt_static_iv_sample$(APP_EXT)     \
	pubnub_objects_secretkey_sample$(APP_EXT)             \
	subscribe_publish_callback_secretkey_sample$(APP_EXT) \
	pubnub_publish_via_post_sample$(APP_EXT)              \
	pubnub_publish_via_post_secretkey_sample$(APP_EXT)    \
	pubnub_sync_grant_token_sample$(APP_EXT)              \
	pubnub_sync_publish_retry$(APP_EXT)                   \
	pubnub_sync_revoke_token_sample$(APP_EXT)             \
	pubnub_sync_sample$(APP_EXT)                          \
	pubnub_sync_secretkey_sample$(APP_EXT)                \
	pubnub_fetch_history_sample$(APP_EXT)                 \
	publish_callback_subloop_sample$(APP_EXT)             \
	publish_queue_callback_subloop$(APP_EXT)              \
	pubnub_callback_sample$(APP_EXT)                      \
	pubnub_callback_subloop_sample$(APP_EXT)              \
	subscribe_publish_callback_sample$(APP_EXT)           \
	subscribe_publish_from_callback$(APP_EXT)             \
	pubnub_console_sync$(APP_EXT)                         \
	pubnub_console_callback$(APP_EXT)                     \
	pubnub_fntest$(APP_EXT)