# Sample application targets.
# Description: Makefile used to declare list of targets to build sample app
#              which demonstrates various features with OpenSSL support.
#
# Note: `nmake` can't handle `/` to `\` replacement in multiline prerequisites
#       so they have been separated from targets for preprocessing.


###############################################################################
#                         Sample application targets                          #
###############################################################################


# ----------------- Samples based on sync PubNub library -----------------

SYNC_CRYPTO_MODULE_SOURCES_ = ../core/samples/pubnub_crypto_module_sample.c
SYNC_CRYPTO_MODULE_SOURCES = $(subst /,$(PATH_SEP),$(SYNC_CRYPTO_MODULE_SOURCES_))
pubnub_crypto_module_sample$(APP_EXT): \
    $(SYNC_CRYPTO_MODULE_SOURCES) \
    pubnub_sync$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

SYNC_CRYPTO_SOURCES_ = ../core/samples/pubnub_crypto_sync_sample.c
SYNC_CRYPTO_SOURCES = $(subst /,$(PATH_SEP),$(SYNC_CRYPTO_SOURCES_))
pubnub_crypto_sync_sample$(APP_EXT): $(SYNC_CRYPTO_SOURCES) pubnub_sync$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

SYNC_OBJECTS_SECRET_KEY_SOURCES_ = ../core/samples/pubnub_objects_secretkey_sample.c
SYNC_OBJECTS_SECRET_KEY_SOURCES = $(subst /,$(PATH_SEP),$(SYNC_OBJECTS_SECRET_KEY_SOURCES_))
pubnub_objects_secretkey_sample$(APP_EXT): \
    $(SYNC_OBJECTS_SECRET_KEY_SOURCES) \
    pubnub_sync$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

SYNC_PUBLISH_VIA_POST_SECRET_KEY_SOURCES_ = \
    ../core/samples/pubnub_publish_via_post_secretkey_sample.c
SYNC_PUBLISH_VIA_POST_SECRET_KEY_SOURCES = $(subst /,$(PATH_SEP),$(SYNC_PUBLISH_VIA_POST_SECRET_KEY_SOURCES_))
pubnub_publish_via_post_secretkey_sample$(APP_EXT): \
    $(SYNC_PUBLISH_VIA_POST_SECRET_KEY_SOURCES) \
    pubnub_sync$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

SYNC_GRANT_TOKEN_SOURCES_ = ../core/samples/pubnub_sync_grant_token_sample.c
SYNC_GRANT_TOKEN_SOURCES = $(subst /,$(PATH_SEP),$(SYNC_GRANT_TOKEN_SOURCES_))
pubnub_sync_grant_token_sample$(APP_EXT): \
    $(SYNC_GRANT_TOKEN_SOURCES) \
    pubnub_sync_dynamiciv$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

SYNC_REVOKE_TOKEN_SOURCES_ = ../core/samples/pubnub_sync_revoke_token_sample.c
SYNC_REVOKE_TOKEN_SOURCES = $(subst /,$(PATH_SEP),$(SYNC_REVOKE_TOKEN_SOURCES_))
pubnub_sync_revoke_token_sample$(APP_EXT): \
    $(SYNC_REVOKE_TOKEN_SOURCES) \
    pubnub_sync_dynamiciv$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

SYNC_SECRET_KEY_SOURCES_ = ../core/samples/pubnub_sync_secretkey_sample.c
SYNC_SECRET_KEY_SOURCES = $(subst /,$(PATH_SEP),$(SYNC_SECRET_KEY_SOURCES_))
pubnub_sync_secretkey_sample$(APP_EXT): \
    $(SYNC_SECRET_KEY_SOURCES) \
    pubnub_sync$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

SYNC_ENCRYPT_DECRYPT_SOURCES_ = ../core/samples/pubnub_encrypt_decrypt_iv_sample.c
SYNC_ENCRYPT_DECRYPT_SOURCES = $(subst /,$(PATH_SEP),$(SYNC_ENCRYPT_DECRYPT_SOURCES_))
pubnub_encrypt_decrypt_static_iv_sample$(APP_EXT): \
    $(SYNC_ENCRYPT_DECRYPT_SOURCES) \
    pubnub_sync$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

pubnub_encrypt_decrypt_dynamic_iv_sample$(APP_EXT): \
    $(SYNC_ENCRYPT_DECRYPT_SOURCES) \
    pubnub_sync_dynamiciv$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)


# --------------- Samples based on callback-based PubNub library --------------

CALLBACK_PUBLISH_SECRET_KEY_SOURCES_ = ../core/samples/subscribe_publish_callback_secretkey_sample.c
CALLBACK_PUBLISH_SECRET_KEY_SOURCES = $(subst /,$(PATH_SEP),$(CALLBACK_PUBLISH_SECRET_KEY_SOURCES_))
subscribe_publish_callback_secretkey_sample$(APP_EXT): \
    $(CALLBACK_PUBLISH_SECRET_KEY_SOURCES) \
    pubnub_callback$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CALLBACK_CPPFLAGS) $(PREREQUISITES) $(LDLIBS)