# Preprocessing flags default values.
# Description: Common for all platforms preprocessing flags default values.


###############################################################################
#                      Default preprocessor flag values                       #
###############################################################################

# Default compiler type.
#
# By default used GCC/Clang-compatible.
DEFAULT_COMPILER_TYPE = $(CC)

# Minimum logger level.
DEFAULT_LOG_MIN_LEVEL = WARNING

# Whether address sanitizer should be enabled or not.
DEFAULT_ASAN = 0

# Whether OpenSSL should be used or not.
DEFAULT_OPENSSL = 0

# Whether should build with for CPP or not.
DEFAULT_WITH_CPP = 0

# Whether C++11 standard should be used for CPP build or not.
DEFAULT_USE_CPP11 = 0

# Whether extern C API should be used or not.
DEFAULT_USE_EXTERN_API = 0

# Whether PubNub should be built with callback interface or not.
DEFAULT_USE_CALLBACK_API = 0

# Whether only publish / subscribe API should be enabled or not.
DEFAULT_ONLY_PUBSUB_API = 0

# Whether compressed response processing should be enabled or not.
DEFAULT_RECEIVE_GZIP_RESPONSE = 1

# Whether message reactions feature should be enabled or not.
DEFAULT_USE_ACTIONS_API = 1

# Whether advanced message persistence feature should be enabled or not.
DEFAULT_USE_ADVANCED_HISTORY = 1

# Whether automatic heartbeat should be enabled or not.
DEFAULT_USE_AUTO_HEARTBEAT = 1

# Whether cryptography feature should be enabled or not.
DEFAULT_USE_CRYPTO_API = 0

# Whether message persistence feature should be enabled or not.
DEFAULT_USE_FETCH_HISTORY = 1

# Whether grant token permissions feature should be enabled or not.
DEFAULT_USE_GRANT_TOKEN = 0

# Whether should enable HTTP body compression or not.
DEFAULT_USE_GZIP_COMPRESSION = 1

# Whether IPv6 connection should be supported or not.
DEFAULT_USE_IPV6 = 1

# Whether to use random IV for legacy crypto module or not.
DEFAULT_USE_LEGACY_CRYPTO_RANDOM_IV = 1

# Whether App Context feature should be enabled or not.
DEFAULT_USE_OBJECTS_API = 1

# Whether request / connection proxy feature should be enabled or not.
DEFAULT_USE_PROXY = 1

# Whether failed request retry feature should be enabled or not.
DEFAULT_USE_RETRY_CONFIGURATION = 0

# Whether token permissions revoke feature should be enabled or not.
DEFAULT_USE_REVOKE_TOKEN = 0

# Whether subscribe event engine feature should be enabled or not.
DEFAULT_USE_SUBSCRIBE_EVENT_ENGINE = 0

# Whether subscribe v2 feature should be enabled or not.
DEFAULT_USE_SUBSCRIBE_V2 = 1

# Whether advanced logger should be enabled or not.
DEFAULT_USE_LOGGER = 1

# Whether default (stdio/platform) logger should be enabled or not.
#
# Important: USE_LOGGER must be enabled to use this.
DEFAULT_USE_DEFAULT_LOGGER = 1