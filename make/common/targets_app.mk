# Sample application targets.
# Description: Makefile used to declare list of targets to build sample app
#              which demonstrates various features.


###############################################################################
#                         Sample application targets                          #
###############################################################################

# ----------------- Samples based on sync PubNub library -----------------

cancel_subscribe_sync_sample$(APP_EXT): \
    $(subst /,$(PATH_SEP),../core/samples/cancel_subscribe_sync_sample.c) \
    pubnub_sync$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

metadata$(APP_EXT): \
    $(subst /,$(PATH_SEP),../core/samples/metadata.c) \
    pubnub_sync$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

pubnub_advanced_history_sample$(APP_EXT): \
    $(subst /,$(PATH_SEP),../core/samples/pubnub_advanced_history_sample.c) \
    pubnub_sync$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

pubnub_fetch_history_sample$(APP_EXT): \
    $(subst /,$(PATH_SEP),../core/samples/pubnub_fetch_history_sample.c) \
    pubnub_sync$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

pubnub_publish_via_post_sample$(APP_EXT): \
    $(subst /,$(PATH_SEP),../core/samples/pubnub_publish_via_post_sample.c) \
    pubnub_sync$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

pubnub_sync_publish_retry$(APP_EXT): \
    $(subst /,$(PATH_SEP),../core/samples/pubnub_sync_publish_retry.c) \
    pubnub_sync$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

pubnub_sync_sample$(APP_EXT): \
    $(subst /,$(PATH_SEP),../core/samples/pubnub_sync_sample.c) \
    pubnub_sync$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

pubnub_sync_secretkey_sample$(APP_EXT): \
    $(subst /,$(PATH_SEP),../core/samples/pubnub_sync_secretkey_sample.c) \
    pubnub_sync$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)


# ----------------- Samples based on callback PubNub library -----------------

publish_callback_subloop_sample$(APP_EXT): \
    $(subst /,$(PATH_SEP),../core/samples/publish_callback_subloop_sample.c) \
    pubnub_callback$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

publish_queue_callback_subloop$(APP_EXT): \
    $(subst /,$(PATH_SEP),../core/samples/publish_queue_callback_subloop.c) \
    pubnub_callback$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

pubnub_callback_sample$(APP_EXT): \
    $(subst /,$(PATH_SEP),../core/samples/pubnub_callback_sample.c) \
    pubnub_callback$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

pubnub_callback_subloop_sample$(APP_EXT): \
    $(subst /,$(PATH_SEP),../core/samples/pubnub_callback_subloop_sample.c) \
    pubnub_callback$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

subscribe_event_engine_sample$(APP_EXT): \
    $(subst /,$(PATH_SEP),../core/samples/subscribe_event_engine_sample.c) \
    pubnub_callback$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CALLBACK_CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

subscribe_publish_callback_sample$(APP_EXT): \
    $(subst /,$(PATH_SEP),../core/samples/subscribe_publish_callback_sample.c) \
    pubnub_callback$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

subscribe_publish_from_callback$(APP_EXT): \
    $(subst /,$(PATH_SEP),../core/samples/subscribe_publish_from_callback.c) \
    pubnub_callback$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)


# --------------- Console based on sync PubNub library --------------

pubnub_console_sync$(APP_EXT): \
    $(subst /,$(PATH_SEP),$(CONSOLE_SOURCE_FILES))               \
    $(subst /,$(PATH_SEP),../core/samples/pubnub_console_sync.c) \
    pubnub_sync$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

pubnub_console_callback$(APP_EXT): \
    $(subst /,$(PATH_SEP),$(CONSOLE_SOURCE_FILES))               \
    $(subst /,$(PATH_SEP),../core/samples/pnc_ops_callback.c) \
    pubnub_callback$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)