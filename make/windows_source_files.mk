# PubNub SDK source files for Windows system.
# Description: Makefile used to declare platform-specific list of PubNub SDK
#              source files.

!include <common/source_files.mk>


###############################################################################
#                          PubNub Core source files                           #
###############################################################################

# PubNub SDK core source files used for all platforms.
SOURCE_FILES = $(CORE_SOURCE_FILES) $(CORE_SOURCE_FILES_WINDOWS)

# Core source files for a synchronous PubNub C-core client version support.
SYNC_SOURCE_FILES = $(SYNC_CORE_SOURCE_FILES)

# Source files for a call-back based PubNub C-core client version support.
CALLBACK_SOURCE_FILES = $(CALLBACK_CORE_SOURCE_FILES)

!if $(WITH_CPP)
    SOURCE_FILES = \
        $(SOURCE_FILES)                  \
        $(CORE_CPP_SOURCE_FILES)         \
        $(CORE_CPP_SOURCE_FILES_WINDOWS)
!else
    SOURCE_FILES = \
        $(SOURCE_FILES)                      \
        $(CORE_NON_CPP_SOURCE_FILES)         \
        $(CORE_NON_CPP_SOURCE_FILES_WINDOWS)
!endif

!if $(OPENSSL)
    # Adding files required to provide OpenSSL support secured connections.
    SOURCE_FILES = \
        $(SOURCE_FILES)                      \
        $(CORE_OPENSSL_SOURCE_FILES)         \
        $(CORE_OPENSSL_SOURCE_FILES_WINDOWS)
    CALLBACK_SOURCE_FILES = \
        $(CALLBACK_SOURCE_FILES)                      \
        $(CALLBACK_CORE_OPENSSL_SOURCE_FILES)         \
        $(CALLBACK_CORE_OPENSSL_SOURCE_FILES_WINDOWS)
!else
    # Adding files required to provide support for regular connections.
    SOURCE_FILES = \
        $(SOURCE_FILES) \
        $(CORE_NON_OPENSSL_SOURCE_FILES)         \
        $(CORE_NON_OPENSSL_SOURCE_FILES_WINDOWS)
    CALLBACK_SOURCE_FILES = \
        $(CALLBACK_SOURCE_FILES)                          \
        $(CALLBACK_CORE_NON_OPENSSL_SOURCE_FILES)         \
        $(CALLBACK_CORE_NON_OPENSSL_SOURCE_FILES_WINDOWS)
!endif


###############################################################################
#                      Add feature-relates source files                       #
###############################################################################

# Published messages content compression feature source files.
!if $(RECEIVE_GZIP_RESPONSE)
    SOURCE_FILES = \
        $(SOURCE_FILES)               \
        $(GZIP_RESPONSE_SOURCE_FILES)
!endif

# Published message reactions feature source files.
!if $(USE_ACTIONS_API)
    SOURCE_FILES = \
        $(SOURCE_FILES)                  \
        $(MESSAGE_REACTION_SOURCE_FILES)
!endif

# Extended history feature source files.
#
# Extended history allows to fetch messages in batch for multiple channels,
# count messages and fetch with additional information.
!if $(USE_ADVANCED_HISTORY)
    SOURCE_FILES = \
        $(SOURCE_FILES)                  \
        $(ADVANCED_HISTORY_SOURCE_FILES)
!endif

# Auto heartbeat feature source files.
#
# Feature used together with `USE_SUBSCRIBE_V2` flag to send periodic heartbeat
# requests to update users' presence on subscribed channels.
!if $(USE_AUTO_HEARTBEAT)
    SOURCE_FILES = \
        $(SOURCE_FILES)                \
        $(AUTO_HEARTBEAT_SOURCE_FILES)

    !if $(OPENSSL)
        SOURCE_FILES = \
            $(SOURCE_FILES)                                \
            $(AUTO_HEARTBEAT_OPENSSL_SOURCE_FILES)         \
            $(AUTO_HEARTBEAT_OPENSSL_SOURCE_FILES_WINDOWS)
    !else
        SOURCE_FILES = \
            $(SOURCE_FILES)                                    \
            $(AUTO_HEARTBEAT_NON_OPENSSL_SOURCE_FILES)         \
            $(AUTO_HEARTBEAT_NON_OPENSSL_SOURCE_FILES_WINDOWS)
    !endif
!endif

# Published / received data encryption and decryption feature source files.
!if $(OPENSSL) && $(USE_CRYPTO_API)
    SOURCE_FILES = \
        $(SOURCE_FILES)        \
        $(CRYPTO_SOURCE_FILES)
!endif

# Custom DNS servers feature source files.
#
# Feature allows to specify custom DNS servers and send query to them.
# Important: Can be used only together with `PUBNUB_CALLBACK_API` flag.
!if $(USE_DNS_SERVERS)
    CALLBACK_SOURCE_FILES = \
        $(CALLBACK_SOURCE_FILES)            \
        $(DNS_SERVERS_SOURCE_FILES)         \
        $(DNS_SERVERS_SOURCE_FILES_WINDOWS)
!endif
!if $(USE_DNS_SERVERS) || $(USE_PROXY)
    CALLBACK_SOURCE_FILES = \
        $(CALLBACK_SOURCE_FILES) \
        $(IPV4_SOURCE_FILES)
    !if $(USE_IPV6)
        CALLBACK_SOURCE_FILES = \
            $(CALLBACK_SOURCE_FILES) \
            $(IPV6_SOURCE_FILES)
    !endif
!endif

# Single channel history feature source files.
!if $(USE_FETCH_HISTORY)
    SOURCE_FILES = \
        $(SOURCE_FILES)               \
        $(FETCH_HISTORY_SOURCE_FILES)
!endif

# Grant token permissions feature source files.
!if $(OPENSSL) && $(USE_GRANT_TOKEN)
    SOURCE_FILES = \
        $(SOURCE_FILES)             \
        $(GRANT_TOKEN_SOURCE_FILES)
!endif

# Compressed service response de-compression feature source files.
!if $(USE_GZIP_COMPRESSION)
    SOURCE_FILES = \
        $(SOURCE_FILES)                  \
        $(GZIP_COMPRESSION_SOURCE_FILES)
!endif

# App Context feature source files.
!if $(USE_OBJECTS_API)
    SOURCE_FILES = \
        $(SOURCE_FILES)             \
        $(APP_CONTEXT_SOURCE_FILES)
!endif

# Custom proxy feature source files.
!if $(USE_PROXY)
    SOURCE_FILES = \
        $(SOURCE_FILES)               \
        $(PROXY_SOURCE_FILES)         \
        $(PROXY_SOURCE_FILES_WINDOWS)
!endif

# Automated failed request retry feature source files.
!if $(USE_RETRY_CONFIGURATION)
    SOURCE_FILES = \
        $(SOURCE_FILES)                     \
        $(RETRY_CONFIGURATION_SOURCE_FILES)
!endif

# Revoke token permissions feature source files.
!if $(OPENSSL) && $(USE_REVOKE_TOKEN)
    SOURCE_FILES = \
        $(SOURCE_FILES)              \
        $(REVOKE_TOKEN_SOURCE_FILES)
!endif

# Subscription event engine feature source files.
!if $(USE_SUBSCRIBE_EVENT_ENGINE)
    SOURCE_FILES = \
        $(SOURCE_FILES)                        \
        $(SUBSCRIBE_EVENT_ENGINE_SOURCE_FILES)
!endif

# Subscribe v2 feature source files.
!if $(USE_SUBSCRIBE_V2)
    SOURCE_FILES = \
        $(SOURCE_FILES)              \
        $(SUBSCRIBE_V2_SOURCE_FILES)
!endif

# There is no suitable functions available in nmake to strip file path,
# so it needs to be done manually.
SOURCE_OBJECT_FILES = $(SOURCE_FILES:../core/c99/=)
SOURCE_OBJECT_FILES = $(SOURCE_OBJECT_FILES:../core/=)
SOURCE_OBJECT_FILES = $(SOURCE_OBJECT_FILES:../cpp/=)
SOURCE_OBJECT_FILES = $(SOURCE_OBJECT_FILES:../lib/base64/=)
SOURCE_OBJECT_FILES = $(SOURCE_OBJECT_FILES:../lib/cbor/=)
SOURCE_OBJECT_FILES = $(SOURCE_OBJECT_FILES:../lib/md5/=)
SOURCE_OBJECT_FILES = $(SOURCE_OBJECT_FILES:../lib/miniz/=)
SOURCE_OBJECT_FILES = $(SOURCE_OBJECT_FILES:../lib/msstopwatch/=)
SOURCE_OBJECT_FILES = $(SOURCE_OBJECT_FILES:../lib/sockets/=)
SOURCE_OBJECT_FILES = $(SOURCE_OBJECT_FILES:../openssl/=)
SOURCE_OBJECT_FILES = $(SOURCE_OBJECT_FILES:../posix/=)
SOURCE_OBJECT_FILES = $(SOURCE_OBJECT_FILES:../qt/=)
SOURCE_OBJECT_FILES = $(SOURCE_OBJECT_FILES:../unreal/=)
SOURCE_OBJECT_FILES = $(SOURCE_OBJECT_FILES:../windows/=)

SYNC_OBJECT_FILES = $(SYNC_SOURCE_FILES:../core/c99/=)
SYNC_OBJECT_FILES = $(SYNC_OBJECT_FILES:../core/=)
SYNC_OBJECT_FILES = $(SYNC_OBJECT_FILES:../cpp/=)
SYNC_OBJECT_FILES = $(SYNC_OBJECT_FILES:../lib/base64/=)
SYNC_OBJECT_FILES = $(SYNC_OBJECT_FILES:../lib/cbor/=)
SYNC_OBJECT_FILES = $(SYNC_OBJECT_FILES:../lib/md5/=)
SYNC_OBJECT_FILES = $(SYNC_OBJECT_FILES:../lib/miniz/=)
SYNC_OBJECT_FILES = $(SYNC_OBJECT_FILES:../lib/msstopwatch/=)
SYNC_OBJECT_FILES = $(SYNC_OBJECT_FILES:../lib/sockets/=)
SYNC_OBJECT_FILES = $(SYNC_OBJECT_FILES:../openssl/=)
SYNC_OBJECT_FILES = $(SYNC_OBJECT_FILES:../posix/=)
SYNC_OBJECT_FILES = $(SYNC_OBJECT_FILES:../qt/=)
SYNC_OBJECT_FILES = $(SYNC_OBJECT_FILES:../unreal/=)
SYNC_OBJECT_FILES = $(SYNC_OBJECT_FILES:../windows/=)

CALLBACK_OBJECT_FILES = $(CALLBACK_SOURCE_FILES:../core/c99/=)
CALLBACK_OBJECT_FILES = $(CALLBACK_OBJECT_FILES:../core/=)
CALLBACK_OBJECT_FILES = $(CALLBACK_OBJECT_FILES:../cpp/=)
CALLBACK_OBJECT_FILES = $(CALLBACK_OBJECT_FILES:../lib/base64/=)
CALLBACK_OBJECT_FILES = $(CALLBACK_OBJECT_FILES:../lib/cbor/=)
CALLBACK_OBJECT_FILES = $(CALLBACK_OBJECT_FILES:../lib/md5/=)
CALLBACK_OBJECT_FILES = $(CALLBACK_OBJECT_FILES:../lib/miniz/=)
CALLBACK_OBJECT_FILES = $(CALLBACK_OBJECT_FILES:../lib/msstopwatch/=)
CALLBACK_OBJECT_FILES = $(CALLBACK_OBJECT_FILES:../lib/sockets/=)
CALLBACK_OBJECT_FILES = $(CALLBACK_OBJECT_FILES:../openssl/=)
CALLBACK_OBJECT_FILES = $(CALLBACK_OBJECT_FILES:../posix/=)
CALLBACK_OBJECT_FILES = $(CALLBACK_OBJECT_FILES:../qt/=)
CALLBACK_OBJECT_FILES = $(CALLBACK_OBJECT_FILES:../unreal/=)
CALLBACK_OBJECT_FILES = $(CALLBACK_OBJECT_FILES:../windows/=)

# Update path separator (if building with GCC).
!if $(PATH_SEP) != "/"
    SOURCE_FILES = $(SOURCE_FILES:/=$(PATH_SEP))
    SYNC_SOURCE_FILES = $(SYNC_SOURCE_FILES:/=$(PATH_SEP))
    CALLBACK_SOURCE_FILES = $(CALLBACK_SOURCE_FILES:/=$(PATH_SEP))
!endif

CPP_OBJECT_FILES = $(SOURCE_OBJECT_FILES:.cpp=.obj)
OBJECT_FILES = $(CPP_OBJECT_FILES:.c=.obj)
SYNC_CPP_OBJECT_FILES = $(SYNC_OBJECT_FILES:.cpp=.obj)
SYNC_OBJECT_FILES = $(SYNC_CPP_OBJECT_FILES:.c=.obj)
CALLBACK_CPP_OBJECT_FILES = $(CALLBACK_OBJECT_FILES:.cpp=.obj)
CALLBACK_SOURCE_FILES = $(CALLBACK_CPP_OBJECT_FILES:.c=.obj)