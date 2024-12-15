# POSIX system preprocessing flags.
# Description: Makefile used to declare platform-specific preprocessing flag
#              defaults.

include ../make/common/preprocessing_defaults.mk


###############################################################################
#                      Default preprocessor flag values                       #
###############################################################################

# Logger level.
LOG_LEVEL ?= $(DEFAULT_LOG_LEVEL)

# Whether address sanitizer should be enabled or not.
ASAN ?= $(DEFAULT_ASAN)

# Whether OpenSSL should be used or not.
OPENSSL ?= $(DEFAULT_OPENSSL)
USE_SSL ?= $(OPENSSL)

# Path relative to the `OPENSSL_ROOT_DIR` which points to the linked OpenSSL
# libraries.
#
# Important: This flag will be used only if OpenSSL root has been specified.
CUSTOM_OPENSSL_LIB_DIR ?=

# Path relative to the `OPENSSL_ROOT_DIR` which points to the included OpenSSL
# headers.
#
# Important: This flag will be used only if OpenSSL root has been specified.
CUSTOM_OPENSSL_INCLUDE_DIR ?=

# OpenSSL root directory
OPENSSL_ROOT_DIR ?=

# Whether GCC should be used for build or not.
WITH_GCC = 1

# Whether should build with for CPP or not.
WITH_CPP ?= $(DEFAULT_WITH_CPP)

# Decide on compiler which should be used to build library and sample
# applications.
COMPILER = $(DEFAULT_COMPILER_TYPE)
ifeq ($(WITH_CPP), 1)
    COMPILER = $(CXX)
    USE_EXTERN_API ?= $(DEFAULT_USE_EXTERN_API)
    USE_CPP11 ?= $(DEFAULT_USE_CPP11)
endif
ifeq ($(COMPILER),)
    $(error "Can't proceed without compiler!")
endif

# Whether PubNub should be built with callback interface or not.
USE_CALLBACK_API ?= $(DEFAULT_USE_CALLBACK_API)

# Whether only publish / subscribe API should be enabled or not.
ONLY_PUBSUB_API ?= $(DEFAULT_ONLY_PUBSUB_API)

# Whether compressed response processing should be enabled or not.
#
# Note: If enabled, will include a third-party library got gzip inflate.
RECEIVE_GZIP_RESPONSE ?= $(DEFAULT_RECEIVE_GZIP_RESPONSE)

# Whether message reactions feature should be enabled or not.
USE_ACTIONS_API ?= $(DEFAULT_USE_ACTIONS_API)

# Whether advanced message persistence feature should be enabled or not.
USE_ADVANCED_HISTORY ?= $(DEFAULT_USE_ADVANCED_HISTORY)

# Whether automatic heartbeat should be enabled or not.
#
# Note: This feature will require additional memory to store PubNub context
# clone and will spin a separate thread to keep an active heartbeat timer.
USE_AUTO_HEARTBEAT ?= $(DEFAULT_USE_AUTO_HEARTBEAT)

# Whether cryptography feature should be enabled or not.
#
# Important: This feature can be used ONLY for build with OpenSSL.
USE_CRYPTO_API ?= $(DEFAULT_USE_CRYPTO_API)
ifeq ($(USE_CRYPTO_API), 1)
	ifeq ($(OPENSSL), 0)
    	$(error "You can't use Crypto API without OpenSSL!")
	endif
endif

# Whether custom DNS servers feature should be enabled or not.
#
# Important: This feature can be used ONLY for build with callback interface.
USE_DNS_SERVERS ?= $(USE_CALLBACK_API)

# Whether message persistence feature should be enabled or not.
USE_FETCH_HISTORY ?= $(DEFAULT_USE_FETCH_HISTORY)

# Whether grant token permissions feature should be enabled or not.
#
# Important: This feature can be used ONLY for build with OpenSSL.
USE_GRANT_TOKEN ?= $(DEFAULT_USE_GRANT_TOKEN)
ifeq ($(USE_GRANT_TOKEN), 1)
	ifeq ($(OPENSSL), 0)
    	$(error "You can't use Grant Token API without OpenSSL!")
	endif
endif

# Whether should enable HTTP body compression or not.
#
# Note: If enabled, will include a third-party library got gzip deflate.
USE_GZIP_COMPRESSION ?= $(DEFAULT_USE_GZIP_COMPRESSION)

# Whether IPv6 connection should be supported or not.
USE_IPV6 ?= $(DEFAULT_USE_IPV6)

# Whether to use random IV for legacy crypto module or not.
#
# Important: This feature can be used ONLY for build with OpenSSL.
USE_LEGACY_CRYPTO_RANDOM_IV ?= $(DEFAULT_USE_LEGACY_CRYPTO_RANDOM_IV)

# Whether App Context feature should be enabled or not.
USE_OBJECTS_API ?= $(DEFAULT_USE_OBJECTS_API)

# Whether request / connection proxy feature should be enabled or not.
USE_PROXY ?= $(DEFAULT_USE_PROXY)

# Whether failed request retry feature should be enabled or not.
USE_RETRY_CONFIGURATION ?= $(DEFAULT_USE_RETRY_CONFIGURATION)

# Whether token permissions revoke feature should be enabled or not.
#
# Important: This feature can be used ONLY for build with OpenSSL.
USE_REVOKE_TOKEN ?= $(DEFAULT_USE_REVOKE_TOKEN)
ifeq ($(USE_REVOKE_TOKEN), 1)
	ifeq ($(OPENSSL), 0)
    	$(error "You can't use Revoke Token API without OpenSSL!")
	endif
endif

# Whether subscribe event engine feature should be enabled or not.
#
# Important: This feature can be used ONLY for build with subscribe v2 enabled.
USE_SUBSCRIBE_EVENT_ENGINE ?= $(DEFAULT_USE_SUBSCRIBE_EVENT_ENGINE)

# Whether subscribe v2 feature should be enabled or not.
USE_SUBSCRIBE_V2 ?= $(DEFAULT_USE_SUBSCRIBE_V2)

# Additional user-provided compiler flags (C/C++).
USER_C_FLAGS ?=
USER_CXX_FLAGS ?=

# Check whether OpenSSL include and lib folders need to be set.
ifeq ($(OPENSSL),1)
	SHOULD_SEARCH_OPENSSL_DIR = 0

	ifeq ($(shell uname),Darwin)
		OPENSSL_CFLAGS := $(shell pkg-config --cflags openssl 2>/dev/null)
		OPENSSL_LIBS := $(shell pkg-config --libs openssl 2>/dev/null)
		ifeq ($(and $(OPENSSL_CFLAGS),$(OPENSSL_LIBS)),)
			SHOULD_SEARCH_OPENSSL_DIR = 1
		endif
	else
		SHOULD_SEARCH_OPENSSL_DIR = 1
	endif

	ifeq ($(SHOULD_SEARCH_OPENSSL_DIR), 1)
		ifeq ($(OPENSSL_ROOT_DIR),)
			ifeq ($(shell test -d "/usr/local/opt/openssl" && echo yes || echo no),yes)
				# Path on macOS with OpenSSL installed with Homebrew.
				OPENSSL_ROOT_DIR = /usr/local/opt/openssl
				CUSTOM_OPENSSL_INCLUDE_DIR = "include"
				CUSTOM_OPENSSL_LIB_DIR = lib
			else ifeq ($(shell test -d "/usr/lib/x86_64-linux-gnu" && echo yes || echo no),yes)
				# Path on GitHub Action Runner (ubuntu-latest image)
				OPENSSL_ROOT_DIR = /usr
				CUSTOM_OPENSSL_INCLUDE_DIR = "include"
				CUSTOM_OPENSSL_LIB_DIR = lib/x86_64-linux-gnu
			endif
		endif
	endif
endif


###############################################################################
#                       Compiler-specific configuration                       #
###############################################################################

# Platform-specific path separator.
#
# This separator used with `subst` function to normalize source file paths.
PATH_SEP = /

# Library creation tool configuration
LIB_TOOL = ar
LIB_CMD = rcs
LIB_OUT_FLAG =

# Built library file extension.
LIB_EXT = .a

# Linker flag.
LINK_FLAG =

# Build sample application extension.
APP_EXT =

# Compiler's flag to enforce C++11 standard.
CPP11_STANDARD_FLAG =
ifeq ($(and $(WITH_CPP),$(USE_CPP11)),1)
    CPP11_STANDARD_FLAG = -std=c++11
endif

# Compiler options flags prefix.
OPTION_PREFIX = -

# Compiler's result output flag.
OUT_FLAG = -o

# Pre-requisites compiler's command modifier.
PREREQUISITES = $^


###############################################################################
#                            Pre-processing flags                             #
###############################################################################

# Included public headers.
INCLUDES_PLATFORM = -I../lib/base64

DEFINES_PLATFORM =
DEFINES_EXTERN_C =
ifeq ($(WITH_CPP),1)
    ifeq ($(USE_EXTERN_API),1)
	    DEFINES_EXTERN_C += -D PUBNUB_USE_EXTERN_C=$(USE_EXTERN_API)
    endif
endif

DEFINES_RANDOM_IV =
INCLUDES_OPENSSL =
ifeq ($(OPENSSL),1)
	DEFINES_RANDOM_IV += -D PUBNUB_RAND_INIT_VECTOR=$(USE_LEGACY_CRYPTO_RANDOM_IV)
	INCLUDES_PLATFORM += -I../openssl

	ifneq ($(and $(OPENSSL_ROOT_DIR),$(CUSTOM_OPENSSL_INCLUDE_DIR)),)
		INCLUDES_OPENSSL += -I$(OPENSSL_ROOT_DIR)/$(CUSTOM_OPENSSL_INCLUDE_DIR)
	else ifeq ($(shell uname),Darwin)
		# Set OpenSSL includes path.
		OPENSSL_CFLAGS := $(shell pkg-config --cflags openssl 2>/dev/null)
		ifneq ($(OPENSSL_CFLAGS),)
			INCLUDES_OPENSSL += $(OPENSSL_CFLAGS)
		endif
	endif
else
	INCLUDES_PLATFORM += -I../posix
endif

# Finish up pre-processing flags configuration with platform-specific data.
include ../make/common/preprocessing.mk