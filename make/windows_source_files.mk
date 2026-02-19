# PubNub SDK source files for Windows system.
# Description: Makefile used to declare platform-specific list of PubNub SDK
#              source files.

!include <../make/common/source_files.mk>


###############################################################################
#                          PubNub Core source files                           #
###############################################################################

# PubNub SDK core source files used for all platforms.
SOURCE_FILES_ = $(CORE_SOURCE_FILES) $(CORE_SOURCE_FILES_WINDOWS)

# Core source files for a synchronous PubNub C-core client version support.
SYNC_SOURCE_FILES_ = $(SYNC_CORE_SOURCE_FILES)

# Source files for a call-back based PubNub C-core client version support.
CALLBACK_SOURCE_FILES_ = $(CALLBACK_CORE_SOURCE_FILES)

# Source files for a ntf runtime selection based PubNub C-core client version support.
NTF_RUNTIME_SELECTION_SOURCE_FILES_ = $(NTF_RUNTIME_SELECTION_CORE_SOURCE_FILES)

!if $(WITH_CPP)
SOURCE_FILES_ = \
    $(SOURCE_FILES_)                 \
    $(CORE_CPP_SOURCE_FILES)         \
    $(CORE_CPP_SOURCE_FILES_WINDOWS)
!else
SOURCE_FILES_ = \
    $(SOURCE_FILES_)                     \
    $(CORE_NON_CPP_SOURCE_FILES)         \
    $(CORE_NON_CPP_SOURCE_FILES_WINDOWS)
!endif

!if $(OPENSSL)
# Adding files required to provide OpenSSL support secured connections.
SOURCE_FILES_ = \
    $(SOURCE_FILES_)                     \
    $(CORE_OPENSSL_SOURCE_FILES)         \
    $(CORE_OPENSSL_SOURCE_FILES_WINDOWS)
CALLBACK_SOURCE_FILES_ = \
    $(CALLBACK_SOURCE_FILES_)                     \
    $(CALLBACK_CORE_OPENSSL_SOURCE_FILES)         \
    $(CALLBACK_CORE_OPENSSL_SOURCE_FILES_WINDOWS)
!else
# Adding files required to provide support for regular connections.
SOURCE_FILES_ = \
    $(SOURCE_FILES_)                         \
    $(CORE_NON_OPENSSL_SOURCE_FILES)         \
    $(CORE_NON_OPENSSL_SOURCE_FILES_WINDOWS)
CALLBACK_SOURCE_FILES_ = \
    $(CALLBACK_SOURCE_FILES_)                         \
    $(CALLBACK_CORE_NON_OPENSSL_SOURCE_FILES)         \
    $(CALLBACK_CORE_NON_OPENSSL_SOURCE_FILES_WINDOWS)
!endif


###############################################################################
#                      Add feature-relates source files                       #
###############################################################################

# Published messages content compression feature source files.
!if $(RECEIVE_GZIP_RESPONSE)
SOURCE_FILES_ = \
    $(SOURCE_FILES_)              \
    $(GZIP_RESPONSE_SOURCE_FILES)
!endif

# Published message reactions feature source files.
!if $(USE_ACTIONS_API)
SOURCE_FILES_ = \
    $(SOURCE_FILES_)                 \
    $(MESSAGE_REACTION_SOURCE_FILES)
!endif

# Extended history feature source files.
#
# Extended history allows to fetch messages in batch for multiple channels,
# count messages and fetch with additional information.
!if $(USE_ADVANCED_HISTORY)
SOURCE_FILES_ = \
    $(SOURCE_FILES_)                 \
    $(ADVANCED_HISTORY_SOURCE_FILES)
!endif

# Auto heartbeat feature source files.
#
# Feature used together with `USE_SUBSCRIBE_V2` flag to send periodic heartbeat
# requests to update users' presence on subscribed channels.
!if $(USE_AUTO_HEARTBEAT)
SOURCE_FILES_ = \
    $(SOURCE_FILES_)               \
    $(AUTO_HEARTBEAT_SOURCE_FILES)

!if $(OPENSSL)
SOURCE_FILES_ = \
    $(SOURCE_FILES_)                               \
    $(AUTO_HEARTBEAT_OPENSSL_SOURCE_FILES)         \
    $(AUTO_HEARTBEAT_OPENSSL_SOURCE_FILES_WINDOWS)
!else
SOURCE_FILES_ = \
    $(SOURCE_FILES_)                                   \
    $(AUTO_HEARTBEAT_NON_OPENSSL_SOURCE_FILES)         \
    $(AUTO_HEARTBEAT_NON_OPENSSL_SOURCE_FILES_WINDOWS)
!endif
!endif

# Published / received data encryption and decryption feature source files.
!if "$(OPENSSL)" =="1" && "$(USE_CRYPTO_API)" == "1"
SOURCE_FILES_ = \
    $(SOURCE_FILES_)       \
    $(CRYPTO_SOURCE_FILES)
!endif

# Custom DNS servers feature source files.
#
# Feature allows to specify custom DNS servers and send query to them.
# Important: Can be used only together with `PUBNUB_CALLBACK_API` flag.
!if $(USE_DNS_SERVERS)
CALLBACK_SOURCE_FILES_ = \
    $(CALLBACK_SOURCE_FILES_)           \
    $(DNS_SERVERS_SOURCE_FILES)         \
    $(DNS_SERVERS_SOURCE_FILES_WINDOWS)
!endif
!if "$(USE_DNS_SERVERS)" == "1" || "$(USE_PROXY)" == "1"
CALLBACK_SOURCE_FILES_ = \
    $(CALLBACK_SOURCE_FILES_) \
    $(IPV4_SOURCE_FILES)
!if $(USE_IPV6)
CALLBACK_SOURCE_FILES_ = \
    $(CALLBACK_SOURCE_FILES_) \
    $(IPV6_SOURCE_FILES)
!endif
!endif

# Single channel history feature source files.
!if $(USE_FETCH_HISTORY)
SOURCE_FILES_ = \
    $(SOURCE_FILES_)              \
    $(FETCH_HISTORY_SOURCE_FILES)
!endif

# Grant token permissions feature source files.
!if "$(OPENSSL)" == "1" && "$(USE_GRANT_TOKEN)" == "1"
SOURCE_FILES_ = \
    $(SOURCE_FILES_)            \
    $(GRANT_TOKEN_SOURCE_FILES)
!endif

# Compressed service response de-compression feature source files.
!if $(USE_GZIP_COMPRESSION)
SOURCE_FILES_ = \
    $(SOURCE_FILES_)                 \
    $(GZIP_COMPRESSION_SOURCE_FILES)
!endif

# App Context feature source files.
!if $(USE_OBJECTS_API)
SOURCE_FILES_ = \
    $(SOURCE_FILES_)            \
    $(APP_CONTEXT_SOURCE_FILES)
!endif

# Custom proxy feature source files.
!if $(USE_PROXY)
SOURCE_FILES_ = \
    $(SOURCE_FILES_)              \
    $(PROXY_SOURCE_FILES)         \
    $(PROXY_SOURCE_FILES_WINDOWS)
!endif

# Automated failed request retry feature source files.
!if $(USE_RETRY_CONFIGURATION)
SOURCE_FILES_ = \
    $(SOURCE_FILES_)                    \
    $(RETRY_CONFIGURATION_SOURCE_FILES)
!endif

# Revoke token permissions feature source files.
!if "$(OPENSSL)" == "1" && "$(USE_REVOKE_TOKEN)" == "1"
SOURCE_FILES_ = \
    $(SOURCE_FILES_)             \
    $(REVOKE_TOKEN_SOURCE_FILES)
!endif

# Subscription event engine feature source files.
!if $(USE_SUBSCRIBE_EVENT_ENGINE)
SOURCE_FILES_ = \
    $(SOURCE_FILES_)                       \
    $(SUBSCRIBE_EVENT_ENGINE_SOURCE_FILES)
!endif

# Subscribe v2 feature source files.
!if $(USE_SUBSCRIBE_V2)
SOURCE_FILES_ = \
    $(SOURCE_FILES_)             \
    $(SUBSCRIBE_V2_SOURCE_FILES)
!endif

# Advanced logger feature source files.
!if $(USE_LOGGER)
SOURCE_FILES_ = \
    $(SOURCE_FILES_)        \
    $(LOGGER_SOURCE_FILES)
!if $(USE_DEFAULT_LOGGER)
SOURCE_FILES_ = \
    $(SOURCE_FILES_)                      \
    $(LOGGER_DEFAULT_STDIO_SOURCE_FILES)
!endif
!endif

# There is no suitable functions available in nmake to strip file path,
# so it needs to be done manually.
SOURCE_OBJECT_FILES_NAMES = $(SOURCE_FILES_:../core/c99/=)
SOURCE_OBJECT_FILES_NAMES = $(SOURCE_OBJECT_FILES_NAMES:../core/=)
SOURCE_OBJECT_FILES_NAMES = $(SOURCE_OBJECT_FILES_NAMES:../cpp/=)
SOURCE_OBJECT_FILES_NAMES = $(SOURCE_OBJECT_FILES_NAMES:../lib/base64/=)
SOURCE_OBJECT_FILES_NAMES = $(SOURCE_OBJECT_FILES_NAMES:../lib/cbor/=)
SOURCE_OBJECT_FILES_NAMES = $(SOURCE_OBJECT_FILES_NAMES:../lib/md5/=)
SOURCE_OBJECT_FILES_NAMES = $(SOURCE_OBJECT_FILES_NAMES:../lib/miniz/=)
SOURCE_OBJECT_FILES_NAMES = $(SOURCE_OBJECT_FILES_NAMES:../lib/msstopwatch/=)
SOURCE_OBJECT_FILES_NAMES = $(SOURCE_OBJECT_FILES_NAMES:../lib/sockets/=)
SOURCE_OBJECT_FILES_NAMES = $(SOURCE_OBJECT_FILES_NAMES:../lib/=)
SOURCE_OBJECT_FILES_NAMES = $(SOURCE_OBJECT_FILES_NAMES:../openssl/=)
SOURCE_OBJECT_FILES_NAMES = $(SOURCE_OBJECT_FILES_NAMES:../posix/=)
SOURCE_OBJECT_FILES_NAMES = $(SOURCE_OBJECT_FILES_NAMES:../qt/=)
SOURCE_OBJECT_FILES_NAMES = $(SOURCE_OBJECT_FILES_NAMES:../unreal/=)
SOURCE_OBJECT_FILES_NAMES = $(SOURCE_OBJECT_FILES_NAMES:../windows/=)

SYNC_OBJECT_FILES_NAMES = $(SYNC_SOURCE_FILES_:../core/c99/=)
SYNC_OBJECT_FILES_NAMES = $(SYNC_OBJECT_FILES_NAMES:../core/=)
SYNC_OBJECT_FILES_NAMES = $(SYNC_OBJECT_FILES_NAMES:../cpp/=)
SYNC_OBJECT_FILES_NAMES = $(SYNC_OBJECT_FILES_NAMES:../lib/base64/=)
SYNC_OBJECT_FILES_NAMES = $(SYNC_OBJECT_FILES_NAMES:../lib/cbor/=)
SYNC_OBJECT_FILES_NAMES = $(SYNC_OBJECT_FILES_NAMES:../lib/md5/=)
SYNC_OBJECT_FILES_NAMES = $(SYNC_OBJECT_FILES_NAMES:../lib/miniz/=)
SYNC_OBJECT_FILES_NAMES = $(SYNC_OBJECT_FILES_NAMES:../lib/msstopwatch/=)
SYNC_OBJECT_FILES_NAMES = $(SYNC_OBJECT_FILES_NAMES:../lib/sockets/=)
SYNC_OBJECT_FILES_NAMES = $(SYNC_OBJECT_FILES_NAMES:../lib/=)
SYNC_OBJECT_FILES_NAMES = $(SYNC_OBJECT_FILES_NAMES:../openssl/=)
SYNC_OBJECT_FILES_NAMES = $(SYNC_OBJECT_FILES_NAMES:../posix/=)
SYNC_OBJECT_FILES_NAMES = $(SYNC_OBJECT_FILES_NAMES:../qt/=)
SYNC_OBJECT_FILES_NAMES = $(SYNC_OBJECT_FILES_NAMES:../unreal/=)
SYNC_OBJECT_FILES_NAMES = $(SYNC_OBJECT_FILES_NAMES:../windows/=)

CALLBACK_OBJECT_FILES_NAMES = $(CALLBACK_SOURCE_FILES_:../core/c99/=)
CALLBACK_OBJECT_FILES_NAMES = $(CALLBACK_OBJECT_FILES_NAMES:../core/=)
CALLBACK_OBJECT_FILES_NAMES = $(CALLBACK_OBJECT_FILES_NAMES:../cpp/=)
CALLBACK_OBJECT_FILES_NAMES = $(CALLBACK_OBJECT_FILES_NAMES:../lib/base64/=)
CALLBACK_OBJECT_FILES_NAMES = $(CALLBACK_OBJECT_FILES_NAMES:../lib/cbor/=)
CALLBACK_OBJECT_FILES_NAMES = $(CALLBACK_OBJECT_FILES_NAMES:../lib/md5/=)
CALLBACK_OBJECT_FILES_NAMES = $(CALLBACK_OBJECT_FILES_NAMES:../lib/miniz/=)
CALLBACK_OBJECT_FILES_NAMES = $(CALLBACK_OBJECT_FILES_NAMES:../lib/msstopwatch/=)
CALLBACK_OBJECT_FILES_NAMES = $(CALLBACK_OBJECT_FILES_NAMES:../lib/sockets/=)
CALLBACK_OBJECT_FILES_NAMES = $(CALLBACK_OBJECT_FILES_NAMES:../lib/=)
CALLBACK_OBJECT_FILES_NAMES = $(CALLBACK_OBJECT_FILES_NAMES:../openssl/=)
CALLBACK_OBJECT_FILES_NAMES = $(CALLBACK_OBJECT_FILES_NAMES:../posix/=)
CALLBACK_OBJECT_FILES_NAMES = $(CALLBACK_OBJECT_FILES_NAMES:../qt/=)
CALLBACK_OBJECT_FILES_NAMES = $(CALLBACK_OBJECT_FILES_NAMES:../unreal/=)
CALLBACK_OBJECT_FILES_NAMES = $(CALLBACK_OBJECT_FILES_NAMES:../windows/=)

NTF_RUNTIME_SELECTION_OBJECT_FILES_NAMES = $(NTF_RUNTIME_SELECTION_SOURCE_FILES_:../core/c99/=)
NTF_RUNTIME_SELECTION_OBJECT_FILES_NAMES = $(NTF_RUNTIME_SELECTION_OBJECT_FILES_NAMES:../core/=)
NTF_RUNTIME_SELECTION_OBJECT_FILES_NAMES = $(NTF_RUNTIME_SELECTION_OBJECT_FILES_NAMES:../cpp/=)
NTF_RUNTIME_SELECTION_OBJECT_FILES_NAMES = $(NTF_RUNTIME_SELECTION_OBJECT_FILES_NAMES:../lib/base64/=)
NTF_RUNTIME_SELECTION_OBJECT_FILES_NAMES = $(NTF_RUNTIME_SELECTION_OBJECT_FILES_NAMES:../lib/cbor/=)
NTF_RUNTIME_SELECTION_OBJECT_FILES_NAMES = $(NTF_RUNTIME_SELECTION_OBJECT_FILES_NAMES:../lib/md5/=)
NTF_RUNTIME_SELECTION_OBJECT_FILES_NAMES = $(NTF_RUNTIME_SELECTION_OBJECT_FILES_NAMES:../lib/miniz/=)
NTF_RUNTIME_SELECTION_OBJECT_FILES_NAMES = $(NTF_RUNTIME_SELECTION_OBJECT_FILES_NAMES:../lib/msstopwatch/=)
NTF_RUNTIME_SELECTION_OBJECT_FILES_NAMES = $(NTF_RUNTIME_SELECTION_OBJECT_FILES_NAMES:../lib/sockets/=)
NTF_RUNTIME_SELECTION_OBJECT_FILES_NAMES = $(NTF_RUNTIME_SELECTION_OBJECT_FILES_NAMES:../lib/=)
NTF_RUNTIME_SELECTION_OBJECT_FILES_NAMES = $(NTF_RUNTIME_SELECTION_OBJECT_FILES_NAMES:../openssl/=)
NTF_RUNTIME_SELECTION_OBJECT_FILES_NAMES = $(NTF_RUNTIME_SELECTION_OBJECT_FILES_NAMES:../posix/=)
NTF_RUNTIME_SELECTION_OBJECT_FILES_NAMES = $(NTF_RUNTIME_SELECTION_OBJECT_FILES_NAMES:../qt/=)
NTF_RUNTIME_SELECTION_OBJECT_FILES_NAMES = $(NTF_RUNTIME_SELECTION_OBJECT_FILES_NAMES:../unreal/=)
NTF_RUNTIME_SELECTION_OBJECT_FILES_NAMES = $(NTF_RUNTIME_SELECTION_OBJECT_FILES_NAMES:../windows/=)
NTF_RUNTIME_SELECTION_OBJECT_FILES_NAMES = $(NTF_RUNTIME_SELECTION_OBJECT_FILES_NAMES:../windows/=)

CPP_OBJECT_FILES = $(subst .cpp,$(OBJ_EXT),$(SOURCE_OBJECT_FILES_NAMES))
OBJECT_FILES = $(subst .c,$(OBJ_EXT),$(CPP_OBJECT_FILES))
OBJECT_FILES = $(strip $(OBJECT_FILES))
SYNC_CPP_OBJECT_FILES = $(subst .cpp,$(OBJ_EXT),$(SYNC_OBJECT_FILES_NAMES))
SYNC_OBJECT_FILES = $(subst .c,$(OBJ_EXT),$(SYNC_CPP_OBJECT_FILES))
SYNC_OBJECT_FILES = $(strip $(SYNC_OBJECT_FILES))
CALLBACK_CPP_OBJECT_FILES = $(subst .cpp,$(OBJ_EXT),$(CALLBACK_OBJECT_FILES_NAMES))
CALLBACK_OBJECT_FILES = $(subst .c,$(OBJ_EXT),$(CALLBACK_CPP_OBJECT_FILES))
CALLBACK_OBJECT_FILES = $(strip $(CALLBACK_OBJECT_FILES))
NTF_RUNTIME_SELECTION_CPP_OBJECT_FILES = $(subst .cpp,$(OBJ_EXT),$(NTF_RUNTIME_SELECTION_OBJECT_FILES_NAMES))
NTF_RUNTIME_SELECTION_OBJECT_FILES = $(subst .c,$(OBJ_EXT),$(NTF_RUNTIME_SELECTION_CPP_OBJECT_FILES))
NTF_RUNTIME_SELECTION_OBJECT_FILES = $(strip $(NTF_RUNTIME_SELECTION_OBJECT_FILES))

# Update path separator (if building with GCC compiler).
!if "$(PATH_SEP)" != "/"
SOURCE_FILES = $(subst /,$(PATH_SEP),$(SOURCE_FILES_))
SYNC_SOURCE_FILES = $(subst /,$(PATH_SEP),$(SYNC_SOURCE_FILES_))
CALLBACK_SOURCE_FILES = $(subst /,$(PATH_SEP),$(CALLBACK_SOURCE_FILES_))
NTF_RUNTIME_SELECTION_SOURCE_FILES = $(subst /,$(PATH_SEP),$(NTF_RUNTIME_SELECTION_SOURCE_FILES_))
!else
SOURCE_FILES = $(SOURCE_FILES_)
SYNC_SOURCE_FILES = $(SYNC_SOURCE_FILES_)
CALLBACK_SOURCE_FILES = $(CALLBACK_SOURCE_FILES_)
NTF_RUNTIME_SELECTION_SOURCE_FILES = $(NTF_RUNTIME_SELECTION_SOURCE_FILES_)
!endif

SOURCE_FILES = $(strip $(SOURCE_FILES))
SYNC_SOURCE_FILES = $(strip $(SYNC_SOURCE_FILES))
CALLBACK_SOURCE_FILES = $(strip $(CALLBACK_SOURCE_FILES))
NTF_RUNTIME_SELECTION_SOURCE_FILES = $(strip $(NTF_RUNTIME_SELECTION_SOURCE_FILES))