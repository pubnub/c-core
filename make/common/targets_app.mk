# Sample application targets.
# Description: Makefile used to declare list of targets to build sample app
#              which demonstrates various features.
#
# Note: `nmake` can't handle `/` to `\` replacement in multiline prerequisites
#       so they have been separated from targets for preprocessing.

###############################################################################
#                         Sample application targets                          #
###############################################################################

# ----------------- Samples based on sync PubNub library -----------------

SYNC_CANCEL_SUBSCRIBE_SOURCES_ = ../core/samples/cancel_subscribe_sync_sample.c
SYNC_CANCEL_SUBSCRIBE_SOURCES = $(subst /,$(PATH_SEP),$(SYNC_CANCEL_SUBSCRIBE_SOURCES_))
cancel_subscribe_sync_sample$(APP_EXT): \
    $(SYNC_CANCEL_SUBSCRIBE_SOURCES) \
    pubnub_sync$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

SYNC_METADATA_SOURCES_ = ../core/samples/metadata.c
SYNC_METADATA_SOURCES = $(subst /,$(PATH_SEP),$(SYNC_METADATA_SOURCES_))
metadata$(APP_EXT): $(SYNC_METADATA_SOURCES) pubnub_sync$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

SYNC_ADVANCED_HISTORY_SOURCES_ = ../core/samples/pubnub_advanced_history_sample.c
SYNC_ADVANCED_HISTORY_SOURCES = $(subst /,$(PATH_SEP),$(SYNC_ADVANCED_HISTORY_SOURCES_))
pubnub_advanced_history_sample$(APP_EXT): \
    $(SYNC_ADVANCED_HISTORY_SOURCES) \
    pubnub_sync$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

SYNC_FETCH_HISTORY_SOURCES_ = ../core/samples/pubnub_fetch_history_sample.c
SYNC_FETCH_HISTORY_SOURCES = $(subst /,$(PATH_SEP),$(SYNC_FETCH_HISTORY_SOURCES_))
pubnub_fetch_history_sample$(APP_EXT): $(SYNC_FETCH_HISTORY_SOURCES) pubnub_sync$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

SYNC_PUBLISH_VIA_POST_SOURCES_ = ../core/samples/pubnub_publish_via_post_sample.c
SYNC_PUBLISH_VIA_POST_SOURCES = $(subst /,$(PATH_SEP),$(SYNC_PUBLISH_VIA_POST_SOURCES_))
pubnub_publish_via_post_sample$(APP_EXT): \
    $(SYNC_PUBLISH_VIA_POST_SOURCES) \
    pubnub_sync$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

SYNC_PUBLISH_RETRY_SOURCES_ = ../core/samples/pubnub_sync_publish_retry.c
SYNC_PUBLISH_RETRY_SOURCES = $(subst /,$(PATH_SEP),$(SYNC_PUBLISH_RETRY_SOURCES_))
pubnub_sync_publish_retry$(APP_EXT): \
    $(SYNC_PUBLISH_RETRY_SOURCES) \
    pubnub_sync$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

SYNC_SAMPLE_SOURCES_ = ../core/samples/pubnub_sync_sample.c
SYNC_SAMPLE_SOURCES = $(subst /,$(PATH_SEP),$(SYNC_SAMPLE_SOURCES_))
pubnub_sync_sample$(APP_EXT): $(SYNC_SAMPLE_SOURCES) pubnub_sync$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

SYNC_OBJECTS_SOURCES_ = ../core/samples/pubnub_objects_api_sample.c
SYNC_OBJECTS_SOURCES = $(subst /,$(PATH_SEP),$(SYNC_OBJECTS_SOURCES_))
pubnub_objects_api_sample$(APP_EXT): $(SYNC_OBJECTS_SOURCES) pubnub_sync$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)


# ----------------- Samples based on callback PubNub library -----------------

CALLBACK_PUBLISH_SUBLOOP_SOURCES_ = ../core/samples/publish_callback_subloop_sample.c
CALLBACK_PUBLISH_SUBLOOP_SOURCES = $(subst /,$(PATH_SEP),$(CALLBACK_PUBLISH_SUBLOOP_SOURCES_))
publish_callback_subloop_sample$(APP_EXT): \
    $(CALLBACK_PUBLISH_SUBLOOP_SOURCES) \
    pubnub_callback$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CALLBACK_CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

CALLBACK_PUBLISH_QUEUE_SUBLOOP_SOURCES_ = ../core/samples/publish_queue_callback_subloop.c
CALLBACK_PUBLISH_QUEUE_SUBLOOP_SOURCES = $(subst /,$(PATH_SEP),$(CALLBACK_PUBLISH_QUEUE_SUBLOOP_SOURCES_))
publish_queue_callback_subloop$(APP_EXT): \
    $(CALLBACK_PUBLISH_QUEUE_SUBLOOP_SOURCES) \
    pubnub_callback$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CALLBACK_CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

CALLBACK_SAMPLE_SOURCES_ = ../core/samples/pubnub_callback_sample.c
CALLBACK_SAMPLE_SOURCES = $(subst /,$(PATH_SEP),$(CALLBACK_SAMPLE_SOURCES_))
pubnub_callback_sample$(APP_EXT): \
    $(CALLBACK_SAMPLE_SOURCES) \
    pubnub_callback$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CALLBACK_CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

CALLBACK_SUBLOOP_SOURCES_ = ../core/samples/pubnub_callback_subloop_sample.c
CALLBACK_SUBLOOP_SOURCES = $(subst /,$(PATH_SEP),$(CALLBACK_SUBLOOP_SOURCES_))
pubnub_callback_subloop_sample$(APP_EXT): \
    $(CALLBACK_SUBLOOP_SOURCES) \
    pubnub_callback$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CALLBACK_CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

CALLBACK_EVENT_ENGINE_SOURCES_ = ../core/samples/subscribe_event_engine_sample.c
CALLBACK_EVENT_ENGINE_SOURCES = $(subst /,$(PATH_SEP),$(CALLBACK_EVENT_ENGINE_SOURCES_))
subscribe_event_engine_sample$(APP_EXT): \
    $(CALLBACK_EVENT_ENGINE_SOURCES) \
    pubnub_callback$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CALLBACK_CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

CALLBACK_SUBSCRIBE_PUBLISH_SOURCES_ = ../core/samples/subscribe_publish_callback_sample.c
CALLBACK_SUBSCRIBE_PUBLISH_SOURCES = $(subst /,$(PATH_SEP),$(CALLBACK_SUBSCRIBE_PUBLISH_SOURCES_))
subscribe_publish_callback_sample$(APP_EXT): \
    $(CALLBACK_SUBSCRIBE_PUBLISH_SOURCES) \
    pubnub_callback$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CALLBACK_CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

CALLBACK_PUBLISH_FROM_CALLBACK_SOURCES_ = ../core/samples/subscribe_publish_from_callback.c
CALLBACK_PUBLISH_FROM_CALLBACK_SOURCES = $(subst /,$(PATH_SEP),$(CALLBACK_PUBLISH_FROM_CALLBACK_SOURCES_))
subscribe_publish_from_callback$(APP_EXT): \
    $(CALLBACK_PUBLISH_FROM_CALLBACK_SOURCES) \
    pubnub_callback$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CALLBACK_CPPFLAGS) $(PREREQUISITES) $(LDLIBS)


# -------------- Samples based on NTF runtime selection -------------
API_ENFORCEMENT_SOURCES_ = ../core/samples/pubnub_api_enforcement_sample.c
API_ENFORCEMENT_SOURCES = $(subst /,$(PATH_SEP),$(API_ENFORCEMENT_SOURCES_))
pubnub_api_enforcement_sample$(APP_EXT): \
	$(API_ENFORCEMENT_SOURCES) \
	pubnub_ntf_sync            \
	pubnub_ntf_callback		   \
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)


# --------------- Console based on sync PubNub library --------------

SYNC_CONSOLE_SOURCES_ = $(CONSOLE_SOURCE_FILES) ../core/samples/console/pnc_ops_sync.c
SYNC_CONSOLE_SOURCES = $(subst /,$(PATH_SEP),$(SYNC_CONSOLE_SOURCES_))
pubnub_console_sync$(APP_EXT): \
    $(SYNC_CONSOLE_SOURCES) \
    pubnub_sync$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

CALLBACK_CONSOLE_SOURCES_ = $(CONSOLE_SOURCE_FILES) ../core/samples/console/pnc_ops_callback.c
CALLBACK_CONSOLE_SOURCES = $(subst /,$(PATH_SEP),$(CALLBACK_CONSOLE_SOURCES_))
pubnub_console_callback$(APP_EXT): \
    $(CALLBACK_CONSOLE_SOURCES) \
    pubnub_callback$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CALLBACK_CPPFLAGS) $(PREREQUISITES) $(LDLIBS)
