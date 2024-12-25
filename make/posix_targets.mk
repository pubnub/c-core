# Targets for POSIX system.
# Description: Makefile used to declare list of targets which can be built for
#              POSIX systems.


###############################################################################
#                                Build targets                                #
###############################################################################

include ../make/common/targets_app.mk

ifeq ($(OPENSSL), 1)
    include ../make/common/targets_app_openssl.mk
endif

# ----------------- Samples based on sync PubNub library -----------------

pubnub_sync_subloop_sample$(APP_EXT): \
    ../core/samples/pubnub_sync_subloop_sample.c \
    pubnub_sync$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

# Test targets
pubnub_fntest$(APP_EXT): \
    ../core/fntest/pubnub_fntest_basic.c   \
    ../core/fntest/pubnub_fntest_medium.c  \
    ../core/fntest/pubnub_fntest.c         \
    ../posix/fntest/pubnub_fntest_posix.c  \
    ../posix/fntest/pubnub_fntest_runner.c \
	pubnub_sync$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

.PHONY: all clean