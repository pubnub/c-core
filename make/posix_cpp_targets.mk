# Sample CPP application targets.
# Description: Makefile used to declare list of targets to build sample app
#              which demonstrates various features.


###############################################################################
#                         Sample application targets                          #
###############################################################################

PUBNUB_FUTRES_SYNC_SOURCE_FILE = ../cpp/pubnub_futres_sync.cpp
PUBNUB_FUTRES_SOURCE_FILE = ../cpp/pubnub_futres_posix.cpp

include ../make/common/targets_cpp_app.mk

ifeq ($(OPENSSL), 1)
    include ../make/common/targets_cpp_app_openssl.mk
endif

# ----------------- Samples based on sync PubNub library -----------------

$(TARGET_BUILD_PATH)pubnub_sync_subloop_sample$(APP_EXT): \
    ../cpp/samples/pubnub_subloop_sample.cpp \
    $(SOURCE_FILES)                          \
    $(SYNC_SOURCE_FILES)                     \
    $(PUBNUB_FUTRES_SYNC_SOURCE_FILE)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)


# ----------------- Samples based on callback PubNub library -----------------

$(TARGET_BUILD_PATH)pubnub_callback_subloop_sample$(APP_EXT): \
    ../cpp/samples/pubnub_subloop_sample.cpp \
    $(SOURCE_FILES)                          \
    $(CALLBACK_SOURCE_FILES)                 \
    $(PUBNUB_FUTRES_SYNC_SOURCE_FILE)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS)  $(CALLBACK_CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

$(TARGET_BUILD_PATH)pubnub_callback_cpp11_subloop_sample$(APP_EXT): \
    ../cpp/samples/pubnub_subloop_sample.cpp \
    $(SOURCE_FILES)                          \
    $(CALLBACK_SOURCE_FILES)                 \
    ../cpp/pubnub_futres_cpp11.cpp
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPP11_STANDARD_FLAG) $(CALLBACK_CPPFLAGS) $(PREREQUISITES) $(LDLIBS)
