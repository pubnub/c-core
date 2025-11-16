# Makefile for PubNub C-core SDK.
# Description: All-in-one make file which show how to build various versions of
#              PubNub SDK which has specific features and link them with application.

# Making CPP version of the PubNub SDK.
WITH_CPP = 1

!include <../make/windows_preprocessing.mk>
!include <../make/windows_source_files.mk>
!include <../make/windows_compiler_linker_flags.mk>


###############################################################################
#                                Build targets                                #
###############################################################################

TARGET_BUILD_PATH =

!include <../make/windows_cpp_targets.mk>


all: \
    $(TARGET_BUILD_PATH)pubnub_callback_cpp11_sample$(APP_EXT)         \
    $(TARGET_BUILD_PATH)pubnub_callback_sample$(APP_EXT)               \
    $(TARGET_BUILD_PATH)subscribe_publish_callback_sample$(APP_EXT)    \
    $(TARGET_BUILD_PATH)pubnub_sync_sample$(APP_EXT)                   \
    $(TARGET_BUILD_PATH)cancel_subscribe_sync_sample$(APP_EXT)         \
    $(TARGET_BUILD_PATH)futres_nesting_sync$(APP_EXT)                  \
    $(TARGET_BUILD_PATH)futres_nesting_callback$(APP_EXT)              \
    $(TARGET_BUILD_PATH)futres_nesting_callback_cpp11$(APP_EXT)        \
    $(TARGET_BUILD_PATH)fntest_runner$(APP_EXT)

clean:
	del *.obj
	del *.exe
	del *.pdb
	del *.il?