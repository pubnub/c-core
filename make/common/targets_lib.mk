# PubNub SDK library targets.
# Description: Makefile used to declare list of targets to build different
#              versions of PubNub SDK.


###############################################################################
#                         PubNub SDK library targets                          #
###############################################################################

# PubNub SDK library with synchronous interface and static initialization
# vector.
# Note: Initialization vector make sense only if `OPENSSL` preprocessing flag
#       is set to `1`.
pubnub_sync$(LIB_EXT): $(SOURCE_FILES) $(SYNC_SOURCE_FILES)
	$(COMPILER) \
		$(OPTION_PREFIX)c                           \
		$(COMPILER_FLAGS)                           \
		$(CPPFLAGS)                                 \
		$(OPTION_PREFIX)U PUBNUB_RAND_INIT_VECTOR   \
		$(OPTION_PREFIX)D PUBNUB_RAND_INIT_VECTOR=0 \
		$(PREREQUISITES)
	$(LIB_TOOL) $(LIB_CMD) $(LIB_OUT_FLAG)$@ $(OBJECT_FILES) $(SYNC_OBJECT_FILES)

# PubNub SDK library with synchronous interface and dynamic initialization
# vector.
# Note: Initialization vector make sense only if `OPENSSL` preprocessing flag
#       is set to `1`.
pubnub_sync_dynamiciv$(LIB_EXT): $(SOURCE_FILES) $(SYNC_SOURCE_FILES)
	$(COMPILER) \
		$(OPTION_PREFIX)c                           \
		$(COMPILER_FLAGS)                           \
		$(CPPFLAGS)                                 \
		$(OPTION_PREFIX)U PUBNUB_RAND_INIT_VECTOR   \
		$(OPTION_PREFIX)D PUBNUB_RAND_INIT_VECTOR=1 \
		$(PREREQUISITES)
	$(LIB_TOOL) $(LIB_CMD) $(LIB_OUT_FLAG)$@ $(OBJECT_FILES) $(SYNC_OBJECT_FILES)

# PubNub SDK library with callback-based interface and dynamic initialization
# vector.
# Note: Initialization vector make sense only if `OPENSSL` preprocessing flag
#       is set to `1`.
pubnub_callback$(LIB_EXT): $(SOURCE_FILES) $(CALLBACK_SOURCE_FILES)
	$(COMPILER) \
		$(OPTION_PREFIX)c                           \
		$(COMPILER_FLAGS)                           \
		$(CALLBACK_CPPFLAGS)                        \
		$(OPTION_PREFIX)U PUBNUB_RAND_INIT_VECTOR   \
		$(OPTION_PREFIX)D PUBNUB_RAND_INIT_VECTOR=0 \
		$(PREREQUISITES)
	$(LIB_TOOL) $(LIB_CMD) $(LIB_OUT_FLAG)$@ $(OBJECT_FILES) $(CALLBACK_OBJECT_FILES)

# PubNub SDK library with callback-based interface and dynamic initialization
# vector.
# Note: Initialization vector make sense only if `OPENSSL` preprocessing flag
#       is set to `1`.
pubnub_callback_dynamiciv$(LIB_EXT): $(SOURCE_FILES) $(CALLBACK_SOURCE_FILES)
	$(COMPILER) -c $(COMPILER_FLAGS) $(CALLBACK_CPPFLAGS) -D PUBNUB_RAND_INIT_VECTOR=1 $(PREREQUISITES)
	$(LIB_TOOL) $(LIB_CMD) $(LIB_OUT_FLAG)$@ $(OBJECT_FILES) $(CALLBACK_OBJECT_FILES)


# PubNub SDK library with NTF runtime selection.
# Note: Initialization vector make sense only if `OPENSSL` preprocessing flag 
# 	 is set to `1`.
pubnub_ntf_runtime_selection$(LIB_EXT): $(SOURCE_FILES) \
		$(SYNC_SOURCE_FILES)     \
		$(CALLBACK_SOURCE_FILES) \
		$(NTF_RUNTIME_SELECTION_SOURCE_FILES)
	$(COMPILER) \
		$(OPTION_PREFIX)c                           \
		$(COMPILER_FLAGS)                           \
		$(CPPFLAGS)                                 \
		$(CALLBACK_CPPFLAGS)                        \
		$(NTF_SELECTION_CPPFLAGS)                   \
		$(OPTION_PREFIX)U PUBNUB_RAND_INIT_VECTOR   \
		$(OPTION_PREFIX)D PUBNUB_RAND_INIT_VECTOR=0 \
		$(PREREQUISITES)
	$(LIB_TOOL) $(LIB_CMD) $(LIB_OUT_FLAG)$@ $(OBJECT_FILES) $(SYNC_OBJECT_FILES) $(CALLBACK_OBJECT_FILES)
# PubNub SDK library with NTF runtime selection and dynamic initialization 
# vector.
# Note: Initialization vector make sense only if `OPENSSL` preprocessing flag 
# 	 is set to `1`.
pubnub_ntf_runtime_selection_dynamiciv$(LIB_EXT): $(SOURCE_FILES) \
		$(SYNC_SOURCE_FILES)     \
		$(CALLBACK_SOURCE_FILES) \
		$(NTF_RUNTIME_SELECTION_SOURCE_FILES)
	$(COMPILER) \
		$(OPTION_PREFIX)c                           \
		$(COMPILER_FLAGS)                           \
		$(CPPFLAGS)                                 \
		$(CALLBACK_CPPFLAGS)                        \
		$(NTF_SELECTION_CPPFLAGS)                   \
		$(OPTION_PREFIX)U PUBNUB_RAND_INIT_VECTOR   \
		$(OPTION_PREFIX)D PUBNUB_RAND_INIT_VECTOR=1 \
		$(PREREQUISITES)
	$(LIB_TOOL) $(LIB_CMD) $(LIB_OUT_FLAG)$@ $(OBJECT_FILES) $(SYNC_OBJECT_FILES) $(CALLBACK_OBJECT_FILES)
