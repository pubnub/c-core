# Sample CPP application targets.
# Description: Makefile used to declare list of targets to build sample app
#              which demonstrates various features.


###############################################################################
#                         Sample application targets                          #
###############################################################################

PUBNUB_FUTRES_SYNC_SOURCE_FILE = ../cpp/pubnub_futres_sync.cpp
PUBNUB_FUTRES_SOURCE_FILE = ../cpp/pubnub_futres_windows.cpp

# Extending compiler flags (preprocessing macro).
COMPILER_FLAGS = \
    $(COMPILER_FLAGS)                                 \
    $(OPTION_PREFIX)D _CRT_SECURE_NO_WARNINGS         \
    $(OPTION_PREFIX)D _WINSOCK_DEPRECATED_NO_WARNINGS

!if $(OPENSSL)
COMPILER_FLAGS = \
    $(COMPILER_FLAGS)                       \
    $(OPTION_PREFIX)D PUBNUB_USE_WIN_SSPI=1
!endif

!include <../make/common/targets_cpp_app.mk>

!if $(OPENSSL)
!include <../make/common/targets_cpp_app_openssl.mk>
!endif