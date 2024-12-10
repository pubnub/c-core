# Sample CPP application targets.
# Description: Makefile used to declare list of targets to build sample app
#              which demonstrates various features.


###############################################################################
#                         Sample application targets                          #
###############################################################################

# ----------------- Samples based on sync PubNub library -----------------

$(TARGET_BUILD_PATH)pubnub_sync_sample$(APP_EXT): \
    $(subst /,$(PATH_SEP),../cpp/samples/pubnub_sample.cpp)  \
    $(SOURCE_FILES)                                          \
    $(SYNC_SOURCE_FILES)                                     \
    $(subst /,$(PATH_SEP),$(PUBNUB_FUTRES_SYNC_SOURCE_FILE))
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LINK_FLAG) $(LDLIBS)

$(TARGET_BUILD_PATH)cancel_subscribe_sync_sample$(APP_EXT): \
    $(subst /,$(PATH_SEP),../cpp/samples/cancel_subscribe_sync_sample.cpp) \
    $(SOURCE_FILES)                                                        \
    $(SYNC_SOURCE_FILES)                                                   \
    $(subst /,$(PATH_SEP),$(PUBNUB_FUTRES_SYNC_SOURCE_FILE))
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LINK_FLAG) $(LDLIBS)

$(TARGET_BUILD_PATH)futres_nesting_sync$(APP_EXT): \
    $(subst /,$(PATH_SEP),../cpp/samples/futres_nesting.cpp) \
    $(SOURCE_FILES)                                          \
    $(SYNC_SOURCE_FILES)                                     \
    $(subst /,$(PATH_SEP),$(PUBNUB_FUTRES_SYNC_SOURCE_FILE))
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LINK_FLAG) $(LDLIBS)

$(TARGET_BUILD_PATH)fntest_runner$(APP_EXT): \
    $(subst /,$(PATH_SEP),../cpp/fntest/pubnub_fntest_runner.cpp) \
    $(SOURCE_FILES)                                               \
    $(SYNC_SOURCE_FILES)                                          \
    $(subst /,$(PATH_SEP),$(PUBNUB_FUTRES_SYNC_SOURCE_FILE))      \
    $(subst /,$(PATH_SEP),../cpp/fntest/pubnub_fntest.cpp)        \
    $(subst /,$(PATH_SEP),../cpp/fntest/pubnub_fntest_basic.cpp)  \
    $(subst /,$(PATH_SEP),../cpp/fntest/pubnub_fntest_medium.cpp)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPP11_STANDARD_FLAG) $(CPPFLAGS) $(PREREQUISITES) $(LINK_FLAG) $(LDLIBS)


# ----------------- Samples based on callback PubNub library -----------------

$(TARGET_BUILD_PATH)pubnub_callback_sample$(APP_EXT): \
    $(subst /,$(PATH_SEP),../cpp/samples/pubnub_sample.cpp) \
    $(SOURCE_FILES)                                         \
    $(CALLBACK_SOURCE_FILES)                                \
    $(subst /,$(PATH_SEP),$(PUBNUB_FUTRES_SOURCE_FILE))
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CALLBACK_CPPFLAGS) $(PREREQUISITES) $(LINK_FLAG) $(LDLIBS)

$(TARGET_BUILD_PATH)pubnub_callback_cpp11_sample$(APP_EXT): \
    $(subst /,$(PATH_SEP),../cpp/samples/pubnub_sample.cpp) \
    $(SOURCE_FILES)                                         \
    $(CALLBACK_SOURCE_FILES)                                \
    $(subst /,$(PATH_SEP),../cpp/pubnub_futres_cpp11.cpp)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPP11_STANDARD_FLAG) $(CALLBACK_CPPFLAGS) $(PREREQUISITES) $(LINK_FLAG) $(LDLIBS)

$(TARGET_BUILD_PATH)subscribe_publish_callback_sample$(APP_EXT): \
    $(subst /,$(PATH_SEP),../cpp/samples/subscribe_publish_callback_sample.cpp) \
    $(SOURCE_FILES)                                      \
    $(CALLBACK_SOURCE_FILES)                             \
    $(subst /,$(PATH_SEP),$(PUBNUB_FUTRES_SOURCE_FILE))
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CALLBACK_CPPFLAGS) $(PREREQUISITES) $(LINK_FLAG) $(LDLIBS)

$(TARGET_BUILD_PATH)futres_nesting_callback$(APP_EXT): \
    $(subst /,$(PATH_SEP),../cpp/samples/futres_nesting.cpp) \
    $(SOURCE_FILES)                                          \
    $(CALLBACK_SOURCE_FILES)                                 \
    $(subst /,$(PATH_SEP),$(PUBNUB_FUTRES_SOURCE_FILE))
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CALLBACK_CPPFLAGS) $(PREREQUISITES) $(LINK_FLAG) $(LDLIBS)

$(TARGET_BUILD_PATH)futres_nesting_callback_cpp11$(APP_EXT): \
    $(subst /,$(PATH_SEP),../cpp/samples/futres_nesting.cpp) \
    $(SOURCE_FILES)                                          \
    $(CALLBACK_SOURCE_FILES)                                 \
    $(subst /,$(PATH_SEP),../cpp/pubnub_futres_cpp11.cpp)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPP11_STANDARD_FLAG) $(CALLBACK_CPPFLAGS) $(PREREQUISITES) $(LINK_FLAG) $(LDLIBS)
