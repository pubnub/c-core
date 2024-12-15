# PubNub SDK source files for POSIX system.
# Description: Makefile used to declare platform-specific list of PubNub SDK
#              source files.

include ../make/common/source_files.mk


###############################################################################
#                          PubNub Core source files                           #
###############################################################################

# PubNub SDK core source files used for all platforms.
SOURCE_FILES = $(CORE_SOURCE_FILES) $(CORE_SOURCE_FILES_POSIX)

# Core source files for a synchronous PubNub C-core client version support.
SYNC_SOURCE_FILES = $(SYNC_CORE_SOURCE_FILES)

# Source files for a call-back based PubNub C-core client version support.
CALLBACK_SOURCE_FILES = $(CALLBACK_CORE_SOURCE_FILES)

ifeq ($(WITH_CPP), 1)
    SOURCE_FILES += \
        $(CORE_CPP_SOURCE_FILES)       \
        $(CORE_CPP_SOURCE_FILES_POSIX)
else
    SOURCE_FILES += \
        $(CORE_NON_CPP_SOURCE_FILES)       \
        $(CORE_NON_CPP_SOURCE_FILES_POSIX)
endif

ifeq ($(OPENSSL),1)
    SOURCE_FILES += \
        $(CORE_OPENSSL_SOURCE_FILES)       \
        $(CORE_OPENSSL_SOURCE_FILES_POSIX)

    CALLBACK_SOURCE_FILES += \
        $(CALLBACK_CORE_OPENSSL_SOURCE_FILES)       \
        $(CALLBACK_CORE_OPENSSL_SOURCE_FILES_POSIX)
else
    SOURCE_FILES += \
        $(CORE_NON_OPENSSL_SOURCE_FILES)       \
        $(CORE_NON_OPENSSL_SOURCE_FILES_POSIX)

    CALLBACK_SOURCE_FILES += \
        $(CALLBACK_CORE_NON_OPENSSL_SOURCE_FILES)       \
        $(CALLBACK_CORE_NON_OPENSSL_SOURCE_FILES_POSIX)
endif

ifeq ($(shell uname),Darwin)
    SOURCE_FILES += ../posix/monotonic_clock_get_time_darwin.c
else
    SOURCE_FILES += ../posix/monotonic_clock_get_time_posix.c
endif


###############################################################################
#                      Add feature-relates source files                       #
###############################################################################

# Published messages content compression feature source files.
ifeq ($(RECEIVE_GZIP_RESPONSE), 1)
    SOURCE_FILES += $(GZIP_RESPONSE_SOURCE_FILES)
endif

# Published message reactions feature source files.
ifeq ($(USE_ACTIONS_API), 1)
    SOURCE_FILES += $(MESSAGE_REACTION_SOURCE_FILES)
endif

# Extended history feature source files.
#
# Extended history allows to fetch messages in batch for multiple channels,
# count messages and fetch with additional information.
ifeq ($(USE_ADVANCED_HISTORY), 1)
    SOURCE_FILES += $(ADVANCED_HISTORY_SOURCE_FILES)
endif

# Auto heartbeat feature source files.
#
# Feature used together with `USE_SUBSCRIBE_V2` flag to send periodic heartbeat
# requests to update users' presence on subscribed channels.
ifeq ($(USE_AUTO_HEARTBEAT), 1)
    SOURCE_FILES += $(AUTO_HEARTBEAT_SOURCE_FILES)
    ifeq ($(OPENSSL), 1)
        SOURCE_FILES += \
            $(AUTO_HEARTBEAT_OPENSSL_SOURCE_FILES)       \
            $(AUTO_HEARTBEAT_OPENSSL_SOURCE_FILES_POSIX)
    else
        SOURCE_FILES += \
            $(AUTO_HEARTBEAT_NON_OPENSSL_SOURCE_FILES)       \
            $(AUTO_HEARTBEAT_NON_OPENSSL_SOURCE_FILES_POSIX)
    endif
endif

# Published / received data encryption and decryption feature source files.
ifeq ($(and $(OPENSSL),$(USE_CRYPTO_API)),1)
    SOURCE_FILES += $(CRYPTO_SOURCE_FILES)
endif

# Custom DNS servers feature source files.
#
# Feature allows to specify custom DNS servers and send query to them.
# Important: Can be used only together with `PUBNUB_CALLBACK_API` flag.
ifeq ($(USE_DNS_SERVERS), 1)
    CALLBACK_SOURCE_FILES += \
        $(DNS_SERVERS_SOURCE_FILES)         \
        $(DNS_SERVERS_SOURCE_FILES_POSIX)
endif
ifeq ($(or $(USE_DNS_SERVERS),$(USE_PROXY)), 1)
    CALLBACK_SOURCE_FILES += $(IPV4_SOURCE_FILES)
    ifeq ($(USE_IPV6), 1)
        CALLBACK_SOURCE_FILES += $(IPV6_SOURCE_FILES)
    endif
endif

# Single channel history feature source files.
ifeq ($(USE_FETCH_HISTORY), 1)
    SOURCE_FILES += $(FETCH_HISTORY_SOURCE_FILES)
endif

# Grant token permissions feature source files.
ifeq ($(and $(OPENSSL),$(USE_GRANT_TOKEN)), 1)
    SOURCE_FILES += $(GRANT_TOKEN_SOURCE_FILES)
endif

# Compressed service response de-compression feature source files.
ifeq ($(USE_GZIP_COMPRESSION), 1)
    SOURCE_FILES += $(GZIP_COMPRESSION_SOURCE_FILES)
endif

# App Context feature source files.
ifeq ($(USE_OBJECTS_API), 1)
    SOURCE_FILES += $(APP_CONTEXT_SOURCE_FILES)
endif

# Custom proxy feature source files.
ifeq ($(USE_PROXY), 1)
    SOURCE_FILES += \
        $(PROXY_SOURCE_FILES)       \
        $(PROXY_SOURCE_FILES_POSIX)
endif

# Automated failed request retry feature source files.
ifeq ($(USE_RETRY_CONFIGURATION), 1)
    SOURCE_FILES += $(RETRY_CONFIGURATION_SOURCE_FILES)
endif

# Revoke token permissions feature source files.
ifeq ($(and $(OPENSSL),$(USE_REVOKE_TOKEN)), 1)
    SOURCE_FILES += $(REVOKE_TOKEN_SOURCE_FILES)
endif

# Subscription event engine feature source files.
ifeq ($(USE_SUBSCRIBE_EVENT_ENGINE), 1)
    SOURCE_FILES += $(SUBSCRIBE_EVENT_ENGINE_SOURCE_FILES)
endif

# Subscribe v2 feature source files.
ifeq ($(USE_SUBSCRIBE_V2), 1)
    SOURCE_FILES += $(SUBSCRIBE_V2_SOURCE_FILES)
endif


# Resulting set of the compiled objects.
OBJECT_FILES = $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(notdir $(SOURCE_FILES))))
SYNC_OBJECT_FILES = $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(notdir $(SYNC_SOURCE_FILES))))
CALLBACK_OBJECT_FILES = $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(notdir $(CALLBACK_SOURCE_FILES))))
