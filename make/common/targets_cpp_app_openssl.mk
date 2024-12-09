# Sample CPP application targets.
# Description: Makefile used to declare list of targets to build sample app
#              which demonstrates various features.


###############################################################################
#                         Sample application targets                          #
###############################################################################

# ----------------- Samples based on sync PubNub library -----------------

$(TARGET_BUILD_PATH)pubnub_sync_grant_sample$(APP_EXT): \
    $(subst /,$(PATH_SEP),../cpp/samples/pubnub_sample_grant_token.cpp) \
    $(SOURCE_FILES)                                                     \
    $(SYNC_SOURCE_FILES)                                                \
    $(subst /,$(PATH_SEP),$(PUBNUB_FUTRES_SYNC_SOURCE_FILE))
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LINK_FLAG) $(LDLIBS)

$(TARGET_BUILD_PATH)pubnub_sync_revoke_sample$(APP_EXT): \
    $(subst /,$(PATH_SEP),../cpp/samples/pubnub_sample_revoke_token.cpp) \
    $(SOURCE_FILES)                                                      \
    $(SYNC_SOURCE_FILES)                                                 \
    $(subst /,$(PATH_SEP),$(PUBNUB_FUTRES_SYNC_SOURCE_FILE))
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LINK_FLAG) $(LDLIBS)

$(TARGET_BUILD_PATH)pubnub_crypto_module_sample$(APP_EXT): \
    $(subst /,$(PATH_SEP),../cpp/samples/pubnub_crypto_module_sample.cpp) \
    $(SOURCE_FILES)                                                       \
    $(SYNC_SOURCE_FILES)                                                  \
    $(subst /,$(PATH_SEP),$(PUBNUB_FUTRES_SYNC_SOURCE_FILE))
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LINK_FLAG) $(LDLIBS)
