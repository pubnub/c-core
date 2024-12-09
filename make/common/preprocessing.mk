# Common preprocessing flags.
# Description: Makefile used to declare platform-independent preprocessing
#              flags.


###############################################################################
#                            Pre-processing flags                             #
###############################################################################

# Included public headers.
INCLUDES = $(OPTION_PREFIX)I.. $(OPTION_PREFIX)I. $(INCLUDES_PLATFORM) $(INCLUDES_OPENSSL)

# Feature flags definition.
DEFINES_COMMON = \
	$(OPTION_PREFIX)D PUBNUB_CRYPTO_API=$(USE_CRYPTO_API)                             \
    $(OPTION_PREFIX)D PUBNUB_LOG_LEVEL=PUBNUB_LOG_LEVEL_$(LOG_LEVEL)                  \
    $(OPTION_PREFIX)D PUBNUB_ONLY_PUBSUB_API=$(ONLY_PUBSUB_API)                       \
    $(OPTION_PREFIX)D PUBNUB_PROXY_API=$(USE_PROXY)                                   \
    $(OPTION_PREFIX)D PUBNUB_RECEIVE_GZIP_RESPONSE=$(RECEIVE_GZIP_RESPONSE)           \
    $(OPTION_PREFIX)D PUBNUB_THREADSAFE=1                                             \
    $(OPTION_PREFIX)D PUBNUB_USE_ACTIONS_API=$(USE_ACTIONS_API)                       \
    $(OPTION_PREFIX)D PUBNUB_USE_ADVANCED_HISTORY=$(USE_ADVANCED_HISTORY)             \
    $(OPTION_PREFIX)D PUBNUB_USE_AUTO_HEARTBEAT=$(USE_AUTO_HEARTBEAT)                 \
    $(OPTION_PREFIX)D PUBNUB_USE_FETCH_HISTORY=$(USE_FETCH_HISTORY)                   \
    $(OPTION_PREFIX)D PUBNUB_USE_GRANT_TOKEN_API=$(USE_GRANT_TOKEN)                   \
    $(OPTION_PREFIX)D PUBNUB_USE_GZIP_COMPRESSION=$(USE_GZIP_COMPRESSION)             \
    $(OPTION_PREFIX)D PUBNUB_USE_IPV6=$(USE_IPV6)                                     \
    $(OPTION_PREFIX)D PUBNUB_USE_OBJECTS_API=$(USE_OBJECTS_API)                       \
    $(OPTION_PREFIX)D PUBNUB_USE_RETRY_CONFIGURATION=$(USE_RETRY_CONFIGURATION)       \
    $(OPTION_PREFIX)D PUBNUB_USE_REVOKE_TOKEN_API=$(USE_REVOKE_TOKEN)                 \
    $(OPTION_PREFIX)D PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE=$(USE_SUBSCRIBE_EVENT_ENGINE) \
    $(OPTION_PREFIX)D PUBNUB_USE_SUBSCRIBE_V2=$(USE_SUBSCRIBE_V2)

# Preprocessing flags for synchronous PubNub library version.
CPPFLAGS = $(INCLUDES) $(DEFINES_COMMON) $(DEFINES_EXTERN_C) $(DEFINES_RANDOM_IV)

# Preprocessing flags for PubNub library with callback interface.
CALLBACK_CPPFLAGS = \
	$(CPPFLAGS)                                                 \
	$(OPTION_PREFIX)D PUBNUB_CALLBACK_API                       \
	$(OPTION_PREFIX)D PUBNUB_SET_DNS_SERVERS=$(USE_DNS_SERVERS)