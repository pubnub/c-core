# Sample CPP application targets.
# Description: Makefile used to declare list of targets to build sample app
#              which demonstrates various features.


###############################################################################
#                         Sample application targets                          #
###############################################################################

# ----------------- Samples based on sync PubNub library -----------------

SYNC_SAMPLE_SOURCES_ = \
    ../cpp/samples/pubnub_sample.cpp  \
    $(SOURCE_FILES)                   \
    $(SYNC_SOURCE_FILES)              \
    $(PUBNUB_FUTRES_SYNC_SOURCE_FILE)
SYNC_SAMPLE_SOURCES = $(subst /,$(PATH_SEP),$(SYNC_SAMPLE_SOURCES_))
$(TARGET_BUILD_PATH)pubnub_sync_sample$(APP_EXT): $(SYNC_SAMPLE_SOURCES)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LINK_FLAG) $(LDLIBS)

SYNC_CANCEL_SUBSCRIBE_SOURCES_ = \
    ../cpp/samples/cancel_subscribe_sync_sample.cpp \
    $(SOURCE_FILES)                                 \
    $(SYNC_SOURCE_FILES)                            \
    $(PUBNUB_FUTRES_SYNC_SOURCE_FILE)
SYNC_CANCEL_SUBSCRIBE_SOURCES = $(subst /,$(PATH_SEP),$(SYNC_CANCEL_SUBSCRIBE_SOURCES_))
$(TARGET_BUILD_PATH)cancel_subscribe_sync_sample$(APP_EXT): \
    $(SYNC_CANCEL_SUBSCRIBE_SOURCES)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LINK_FLAG) $(LDLIBS)

SYNC_FUTRES_NESTING_SOURCES_ = \
    ../cpp/samples/futres_nesting.cpp \
    $(SOURCE_FILES)                   \
    $(SYNC_SOURCE_FILES)              \
    $(PUBNUB_FUTRES_SYNC_SOURCE_FILE)
SYNC_FUTRES_NESTING_SOURCES = $(subst /,$(PATH_SEP),$(SYNC_FUTRES_NESTING_SOURCES_))
$(TARGET_BUILD_PATH)futres_nesting_sync$(APP_EXT): $(SYNC_FUTRES_NESTING_SOURCES)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LINK_FLAG) $(LDLIBS)

SYNC_FNTEST_RUNNER_SOURCES_ = \
    ../cpp/fntest/pubnub_fntest_runner.cpp \
    $(SOURCE_FILES)                        \
    $(SYNC_SOURCE_FILES)                   \
    $(PUBNUB_FUTRES_SYNC_SOURCE_FILE)      \
    ../cpp/fntest/pubnub_fntest.cpp        \
    ../cpp/fntest/pubnub_fntest_basic.cpp  \
    ../cpp/fntest/pubnub_fntest_medium.cpp
SYNC_FNTEST_RUNNER_SOURCES = $(subst /,$(PATH_SEP),$(SYNC_FNTEST_RUNNER_SOURCES_))
$(TARGET_BUILD_PATH)fntest_runner$(APP_EXT): $(SYNC_FNTEST_RUNNER_SOURCES)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPP11_STANDARD_FLAG) $(CPPFLAGS) $(PREREQUISITES) $(LINK_FLAG) $(LDLIBS)


# ----------------- Samples based on callback PubNub library -----------------

CALLBACK_SAMPLE_SOURCES_ = \
    ../cpp/samples/pubnub_sample.cpp \
    $(SOURCE_FILES)                  \
    $(CALLBACK_SOURCE_FILES)         \
    $(PUBNUB_FUTRES_SOURCE_FILE)
CALLBACK_SAMPLE_SOURCES = $(subst /,$(PATH_SEP),$(CALLBACK_SAMPLE_SOURCES_))
$(TARGET_BUILD_PATH)pubnub_callback_sample$(APP_EXT): \
    $(CALLBACK_SAMPLE_SOURCES)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CALLBACK_CPPFLAGS) $(PREREQUISITES) $(LINK_FLAG) $(LDLIBS)

CALLBACK_SAMPLE_CPP11_SOURCES_ = \
    ../cpp/samples/pubnub_sample.cpp  \
    $(SOURCE_FILES)                   \
    $(CALLBACK_SOURCE_FILES)          \
    ../cpp/pubnub_futres_cpp11.cpp
CALLBACK_SAMPLE_CPP11_SOURCES = $(subst /,$(PATH_SEP),$(CALLBACK_SAMPLE_CPP11_SOURCES_))
$(TARGET_BUILD_PATH)pubnub_callback_cpp11_sample$(APP_EXT): \
    $(CALLBACK_SAMPLE_CPP11_SOURCES)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPP11_STANDARD_FLAG) $(CALLBACK_CPPFLAGS) $(PREREQUISITES) $(LINK_FLAG) $(LDLIBS)

CALLBACK_SUBSCRIBE_PUBLISH_SOURCES_ = \
    ../cpp/samples/subscribe_publish_callback_sample.cpp  \
    $(SOURCE_FILES)                                       \
    $(CALLBACK_SOURCE_FILES)                              \
    $(PUBNUB_FUTRES_SOURCE_FILE)
CALLBACK_SUBSCRIBE_PUBLISH_SOURCES = $(subst /,$(PATH_SEP),$(CALLBACK_SUBSCRIBE_PUBLISH_SOURCES_))
$(TARGET_BUILD_PATH)subscribe_publish_callback_sample$(APP_EXT): \
    $(CALLBACK_SUBSCRIBE_PUBLISH_SOURCES)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CALLBACK_CPPFLAGS) $(PREREQUISITES) $(LINK_FLAG) $(LDLIBS)

CALLBACK_FUTRES_NESTING_SOURCES_ = \
    ../cpp/samples/futres_nesting.cpp  \
    $(SOURCE_FILES)                                       \
    $(CALLBACK_SOURCE_FILES)                              \
    $(PUBNUB_FUTRES_SOURCE_FILE)
CALLBACK_FUTRES_NESTING_SOURCES = $(subst /,$(PATH_SEP),$(CALLBACK_FUTRES_NESTING_SOURCES_))
$(TARGET_BUILD_PATH)futres_nesting_callback$(APP_EXT): \
    $(CALLBACK_FUTRES_NESTING_SOURCES)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CALLBACK_CPPFLAGS) $(PREREQUISITES) $(LINK_FLAG) $(LDLIBS)

CALLBACK_FUTRES_NESTING_CPP11_SOURCES_ = \
    ../cpp/samples/futres_nesting.cpp \
    $(SOURCE_FILES)                   \
    $(CALLBACK_SOURCE_FILES)          \
    ../cpp/pubnub_futres_cpp11.cpp
CALLBACK_FUTRES_NESTING_CPP11_SOURCES = $(subst /,$(PATH_SEP),$(CALLBACK_FUTRES_NESTING_CPP11_SOURCES_))
$(TARGET_BUILD_PATH)futres_nesting_callback_cpp11$(APP_EXT): \
    $(CALLBACK_FUTRES_NESTING_CPP11_SOURCES)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPP11_STANDARD_FLAG) $(CALLBACK_CPPFLAGS) $(PREREQUISITES) $(LINK_FLAG) $(LDLIBS)
