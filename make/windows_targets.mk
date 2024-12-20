# Targets for Windows system.
# Description: Makefile used to declare list of targets which can be built for
#              Windows systems.
#
# Note: `nmake` can't handle `/` to `\` replacement in multiline prerequisites
#       so they have been separated from targets for preprocessing.

!include <../make/common/targets_app.mk>


###############################################################################
#                                Build targets                                #
###############################################################################

# --------------- Tests based on sync PubNub library --------------

FNTEST_SOURCES_ = \
    ../core/fntest/pubnub_fntest.c            \
    ../core/fntest/pubnub_fntest_basic.c      \
    ../core/fntest/pubnub_fntest_medium.c     \
    ../windows/fntest/pubnub_fntest_runner.c  \
    ../windows/fntest/pubnub_fntest_windows.c
FNTEST_SOURCES = $(subst /,$(PATH_SEP),$(FNTEST_SOURCES_))
pubnub_fntest$(APP_EXT): $(FNTEST_SOURCES) pubnub_sync$(LIB_EXT)
	$(COMPILER) $(OUT_FLAG)$@ $(COMPILER_FLAGS) $(CPPFLAGS) $(PREREQUISITES) $(LDLIBS)

clean:
	del *.exe
	del *.obj
	del *.pdb
	del *.il?
	del *.lib
	del *.c~
	del *.h~
	del *~

.PHONY: all clean