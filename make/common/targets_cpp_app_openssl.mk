# Sample CPP application targets.
# Description: Makefile used to declare list of targets to build sample app
#              which demonstrates various features.


###############################################################################
#                         Sample application targets                          #
###############################################################################

# ----------------- Samples based on sync PubNub library -----------------

SYNC_GRANT_SOURCES_ = \
    ../cpp/samples/pubnub_sample_grant_token.cpp \
    $(SOURCE_FILES)                              \
    $(SYNC_SOURCE_FILES)                         \
    $(PUBNUB_FUTRES_SYNC_SOURCE_FILE)
SYNC_GRANT_SOURCES = $(subst /,$(PATH_SEP),$(SYNC_GRANT_SOURCES_))
$(TARGET_BUILD_PATH)pubnub_sync_grant_sample$(APP_EXT): $(SYNC_GRANT_SOURCES)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LINK_FLAG) $(LDLIBS)

SYNC_REVOKE_SOURCES_ = \
    ../cpp/samples/pubnub_sample_revoke_token.cpp \
    $(SOURCE_FILES)                               \
    $(SYNC_SOURCE_FILES)                          \
    $(PUBNUB_FUTRES_SYNC_SOURCE_FILE)
SYNC_REVOKE_SOURCES = $(subst /,$(PATH_SEP),$(SYNC_REVOKE_SOURCES_))
$(TARGET_BUILD_PATH)pubnub_sync_revoke_sample$(APP_EXT): \
    $(SYNC_REVOKE_SOURCES)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LINK_FLAG) $(LDLIBS)

SYNC_CRYPTO_MODULE_SOURCES_ = \
    ../cpp/samples/pubnub_crypto_module_sample.cpp \
    $(SOURCE_FILES)                               \
    $(SYNC_SOURCE_FILES)                          \
    $(PUBNUB_FUTRES_SYNC_SOURCE_FILE)
SYNC_CRYPTO_MODULE_SOURCES = $(subst /,$(PATH_SEP),$(SYNC_CRYPTO_MODULE_SOURCES_))
$(TARGET_BUILD_PATH)pubnub_crypto_module_sample$(APP_EXT): \
    $(SYNC_CRYPTO_MODULE_SOURCES)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LINK_FLAG) $(LDLIBS)
