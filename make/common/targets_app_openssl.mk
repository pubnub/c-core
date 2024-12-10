# Sample application targets.
# Description: Makefile used to declare list of targets to build sample app
#              which demonstrates various features with OpenSSL support.


###############################################################################
#                         Sample application targets                          #
###############################################################################


# ----------------- Samples based on sync PubNub library -----------------

pubnub_crypto_module_sample$(APP_EXT): \
    $(subst /,$(PATH_SEP),../core/samples/pubnub_crypto_module_sample.c) \
    pubnub_sync$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

pubnub_crypto_sync_sample$(APP_EXT): \
    $(subst /,$(PATH_SEP),../core/samples/pubnub_crypto_sync_sample.c) \
    pubnub_sync$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

pubnub_objects_secretkey_sample$(APP_EXT): \
    $(subst /,$(PATH_SEP),../core/samples/pubnub_objects_secretkey_sample.c) \
    pubnub_sync$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

pubnub_publish_via_post_secretkey_sample$(APP_EXT): \
    $(subst /,$(PATH_SEP),../core/samples/pubnub_publish_via_post_secretkey_sample.c) \
    pubnub_sync$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

pubnub_sync_grant_token_sample$(APP_EXT): \
    $(subst /,$(PATH_SEP),../core/samples/pubnub_sync_grant_token_sample.c) \
    pubnub_sync_dynamiciv$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

pubnub_sync_revoke_token_sample$(APP_EXT): \
    $(subst /,$(PATH_SEP),../core/samples/pubnub_sync_revoke_token_sample.c) \
    pubnub_sync_dynamiciv$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

pubnub_encrypt_decrypt_static_iv_sample$(APP_EXT): \
    $(subst /,$(PATH_SEP),../core/samples/pubnub_encrypt_decrypt_iv_sample.c) \
    pubnub_sync$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

pubnub_encrypt_decrypt_dynamic_iv_sample$(APP_EXT): \
    $(subst /,$(PATH_SEP),../core/samples/pubnub_encrypt_decrypt_iv_sample.c) \
    pubnub_sync_dynamiciv$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)


# --------------- Samples based on callback-based PubNub library --------------

subscribe_publish_callback_secretkey_sample$(APP_EXT): \
    $(subst /,$(PATH_SEP),../core/samples/subscribe_publish_callback_secretkey_sample.c) \
    pubnub_callback$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)