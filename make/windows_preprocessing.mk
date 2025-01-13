# Windows preprocessing flags.
# Description: Makefile used to declare platform-specific preprocessing flag
#              defaults.

!include <../make/common/preprocessing_defaults.mk>


###############################################################################
#                      Default preprocessor flag values                       #
###############################################################################

# Logger level.
!ifndef LOG_LEVEL
LOG_LEVEL = $(DEFAULT_LOG_LEVEL)
!endif

# Whether address sanitizer should be enabled or not.
!ifndef ASAN
ASAN = $(DEFAULT_ASAN)
!endif

# Whether GCC should be used for build or not.
!ifndef WITH_GCC
WITH_GCC = 0
!endif

!if $(WITH_GCC)
# Platform-specific path separator.
#
# This separator used with `subst` function to normalize source file paths.
PATH_SEP = /
!else
# Platform-specific path separator.
#
# This separator used with `subst` function to normalize source file paths.
PATH_SEP = \\
!endif

# Whether OpenSSL should be used or not.
!ifndef OPENSSL
OPENSSL = $(DEFAULT_OPENSSL)
!endif
!ifndef USE_SSL
USE_SSL = $(OPENSSL)
!endif

# Path relative to the `OPENSSL_ROOT_DIR` which points to the linked OpenSSL
# libraries.
#
# Important: This flag will be used only if OpenSSL root has been specified.
!ifndef CUSTOM_OPENSSL_LIB_DIR
CUSTOM_OPENSSL_LIB_DIR = lib
!endif

# Path relative to the `OPENSSL_ROOT_DIR` which points to the included OpenSSL
# headers.
#
# Important: This flag will be used only if OpenSSL root has been specified.
!ifndef CUSTOM_OPENSSL_INCLUDE_DIR
CUSTOM_OPENSSL_INCLUDE_DIR = include
!endif

# OpenSSL root directory
!ifdef OPENSSLPATH
OPENSSL_ROOT_DIR = $(OPENSSLPATH)
!elseifndef OPENSSL_ROOT_DIR
# Path will be updated if build is done using non-GCC compiler.
OPENSSL_ROOT_DIR = C:$(PATH_SEP)OpenSSL-Win32
!endif

# Whether should build with for CPP or not.
!ifndef WITH_CPP
WITH_CPP = $(DEFAULT_WITH_CPP)
!endif

# Decide on compiler which should be used to build library and sample
# applications.
COMPILER = $(DEFAULT_COMPILER_TYPE)
!if $(WITH_CPP)
COMPILER = $(CXX)
!ifndef $(USE_EXTERN_API)
USE_EXTERN_API = $(DEFAULT_USE_EXTERN_API)
!endif
!ifndef $(USE_CPP11)
USE_CPP11 = $(DEFAULT_USE_CPP11)
!endif
!endif
!if "$(COMPILER)" == ""
!error "Can't proceed without compiler!"
!endif

# Whether PubNub should be built with callback interface or not.
!ifndef USE_CALLBACK_API
USE_CALLBACK_API = $(DEFAULT_USE_CALLBACK_API)
!endif

# Whether only publish / subscribe API should be enabled or not.
!ifndef ONLY_PUBSUB_API
ONLY_PUBSUB_API = $(DEFAULT_ONLY_PUBSUB_API)
!endif

# Whether compressed response processing should be enabled or not.
#
# Note: If enabled, will include a third-party library got gzip inflate.
!ifndef RECEIVE_GZIP_RESPONSE
RECEIVE_GZIP_RESPONSE = $(DEFAULT_RECEIVE_GZIP_RESPONSE)
!endif

# Whether message reactions feature should be enabled or not.
!ifndef USE_ACTIONS_API
USE_ACTIONS_API = $(DEFAULT_USE_ACTIONS_API)
!endif

# Whether advanced message persistence feature should be enabled or not.
!ifndef USE_ADVANCED_HISTORY
USE_ADVANCED_HISTORY = $(DEFAULT_USE_ADVANCED_HISTORY)
!endif

# Whether automatic heartbeat should be enabled or not.
#
# Note: This feature will require additional memory to store PubNub context
# clone and will spin a separate thread to keep an active heartbeat timer.
!ifndef USE_AUTO_HEARTBEAT
USE_AUTO_HEARTBEAT = $(DEFAULT_USE_AUTO_HEARTBEAT)
!endif

# Whether cryptography feature should be enabled or not.
#
# Important: This feature can be used ONLY for build with OpenSSL.
!ifndef USE_CRYPTO_API
USE_CRYPTO_API = $(DEFAULT_USE_CRYPTO_API)
!endif
!if $(USE_CRYPTO_API)
!if !$(OPENSSL)
!error "You can't use Crypto API without OpenSSL!"
!endif
!endif

# Whether custom DNS servers feature should be enabled or not.
#
# Important: This feature can be used ONLY for build with callback interface.
!ifndef USE_DNS_SERVERS
USE_DNS_SERVERS = $(USE_CALLBACK_API)
!endif

# Whether message persistence feature should be enabled or not.
!ifndef USE_FETCH_HISTORY
USE_FETCH_HISTORY = $(DEFAULT_USE_FETCH_HISTORY)
!endif

# Whether grant token permissions feature should be enabled or not.
#
# Important: This feature can be used ONLY for build with OpenSSL.
!ifndef USE_GRANT_TOKEN
USE_GRANT_TOKEN = $(DEFAULT_USE_GRANT_TOKEN)
!endif
!if $(USE_GRANT_TOKEN) == 1
!if $(OPENSSL) == 0
!error "You can't use Grant Token API without OpenSSL!"
!endif
!endif

# Whether should enable HTTP body compression or not.
#
# Note: If enabled, will include a third-party library got gzip deflate.
!ifndef USE_GZIP_COMPRESSION
USE_GZIP_COMPRESSION = $(DEFAULT_USE_GZIP_COMPRESSION)
!endif

# Whether IPv6 connection should be supported or not.
!ifndef USE_IPV6
USE_IPV6 = $(DEFAULT_USE_IPV6)
!endif

# Whether to use random IV for legacy crypto module or not.
#
# Important: This feature can be used ONLY for build with OpenSSL.
!ifndef USE_LEGACY_CRYPTO_RANDOM_IV
USE_LEGACY_CRYPTO_RANDOM_IV = $(DEFAULT_USE_LEGACY_CRYPTO_RANDOM_IV)
!endif

# Whether App Context feature should be enabled or not.
!ifndef USE_OBJECTS_API
USE_OBJECTS_API = $(DEFAULT_USE_OBJECTS_API)
!endif

# Whether request / connection proxy feature should be enabled or not.
!ifndef USE_PROXY
USE_PROXY = $(DEFAULT_USE_PROXY)
!endif

# Whether failed request retry feature should be enabled or not.
!ifndef USE_RETRY_CONFIGURATION
USE_RETRY_CONFIGURATION = $(DEFAULT_USE_RETRY_CONFIGURATION)
!endif

# Whether token permissions revoke feature should be enabled or not.
#
# Important: This feature can be used ONLY for build with OpenSSL.
!ifndef USE_REVOKE_TOKEN
USE_REVOKE_TOKEN = $(DEFAULT_USE_REVOKE_TOKEN)
!endif
!if $(USE_REVOKE_TOKEN)
!if !$(OPENSSL)
!error "You can't use Revoke Token API without OpenSSL!"
!endif
!endif

# Whether subscribe event engine feature should be enabled or not.
#
# Important: This feature can be used ONLY for build with subscribe v2 enabled.
!ifndef USE_SUBSCRIBE_EVENT_ENGINE
USE_SUBSCRIBE_EVENT_ENGINE = $(DEFAULT_USE_SUBSCRIBE_EVENT_ENGINE)
!endif

# Whether subscribe v2 feature should be enabled or not.
!ifndef USE_SUBSCRIBE_V2
USE_SUBSCRIBE_V2 = $(DEFAULT_USE_SUBSCRIBE_V2)
!endif

# Whether user defined callback for logging should be enabled or not.
!ifndef USE_LOG_CALLBACK
USE_LOG_CALLBACK = $(DEFAULT_USE_LOG_CALLBACK)
!endif

# Additional user-provided compiler flags (C/C++).
!ifndef USER_C_FLAGS
USER_C_FLAGS =
!endif
!ifndef USER_CXX_FLAGS
USER_CXX_FLAGS =
!endif


###############################################################################
#                       Compiler-specific configuration                       #
###############################################################################

CPP11_STANDARD_FLAG =

# Decide on prefix which should be used for options (`/D` or `-D`),
!if $(WITH_GCC)
OBJ_EXT = .o

# Compiler options flags prefix.
OPTION_PREFIX = -
LIB_TOOL = ar
LIB_CMD = rcs
LIB_OUT_FLAG =

# Built library file extension.
LIB_EXT = .a

# Linker flag.
LINK_FLAG =

# Build sample application extension.
APP_EXT =

# Compiler's result output flag.
OUT_FLAG = -o

# Compiler's flag to enforce C++11 standard.
!if "$(WITH_CPP)" == "1" && "$(USE_CPP11)" == "1"
CPP11_STANDARD_FLAG = -std=c++11
!endif
!else
OBJ_EXT = .obj

# Compiler options flags prefix.
OPTION_PREFIX = /

# Library creation tool configuration
LIB_TOOL = lib
LIB_CMD =
LIB_OUT_FLAG = -OUT:

# Built library file extension.
LIB_EXT = .lib

# Linker flag.
LINK_FLAG =

# Build sample application extension.
APP_EXT = .exe

# Compiler's result output flag.
OUT_FLAG = /Fe

# Compiler's flag to enforce C++11 standard.
!if $(WITH_CPP)
# Linker flag.
LINK_FLAG = /link
!if $(USE_CPP11)
CPP11_STANDARD_FLAG = /std:c++11
!endif
!endif
!endif

# Pre-requisites compiler's command modifier.
PREREQUISITES = $**


###############################################################################
#                            Pre-processing flags                             #
###############################################################################

# Included public headers.
INCLUDES_PLATFORM = \
    $(OPTION_PREFIX)I..$(PATH_SEP)core$(PATH_SEP)c99   \
    $(OPTION_PREFIX)I..$(PATH_SEP)lib$(PATH_SEP)base64

DEFINES_PLATFORM = $(OPTION_PREFIX)D _WINSOCKAPI_
DEFINES_EXTERN_C =
!if "$(WITH_CPP)" == "1" && "$(USE_EXTERN_API)" == "1"
DEFINE_EXTERN_C = $(OPTION_PREFIX)D PUBNUB_USE_EXTERN_C=$(USE_EXTERN_API)
!endif

DEFINES_RANDOM_IV =
INCLUDES_OPENSSL =
!if $(OPENSSL)
DEFINE_RANDOM_IV = $(OPTION_PREFIX)D PUBNUB_RAND_INIT_VECTOR=$(USE_LEGACY_CRYPTO_RANDOM_IV)
INCLUDES_OPENSSL = $(OPTION_PREFIX)I..$(PATH_SEP)openssl

!if "$(OPENSSL_ROOT_DIR)" != "" && "$(CUSTOM_OPENSSL_INCLUDE_DIR)" != ""
OPENSSL_ROOT_DIR_ = $(subst /,$(PATH_SEP),$(OPENSSL_ROOT_DIR))
CUSTOM_OPENSSL_INCLUDE_DIR_ = $(subst /,$(PATH_SEP),$(CUSTOM_OPENSSL_INCLUDE_DIR))
INCLUDES_OPENSSL = $(INCLUDES_OPENSSL) \
    $(OPTION_PREFIX)I"$(OPENSSL_ROOT_DIR_)$(PATH_SEP)$(CUSTOM_OPENSSL_INCLUDE_DIR_)"
!endif
!else
INCLUDES_PLATFORM = $(INCLUDES_PLATFORM) $(OPTION_PREFIX)I..$(PATH_SEP)windows
!endif

# Finish up pre-processing flags configuration with platform-specific data.
!include <../make/common/preprocessing.mk>