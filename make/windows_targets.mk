# Targets for Windows system.
# Description: Makefile used to declare list of targets which can be built for
#              Windows systems.

!include <common/targets_app.mk>


###############################################################################
#                                Build targets                                #
###############################################################################

# --------------- Tests based on sync PubNub library --------------

pubnub_fntest$(APP_EXT): \
    $(subst /,$(PATH_SEP),../core/fntest/pubnub_fntest.c)            \
    $(subst /,$(PATH_SEP),../core/fntest/pubnub_fntest_basic.c)      \
    $(subst /,$(PATH_SEP),../core/fntest/pubnub_fntest_medium.c)     \
    $(subst /,$(PATH_SEP),../windows/fntest/pubnub_fntest_runner.c)  \
    $(subst /,$(PATH_SEP),../windows/fntest/pubnub_fntest_windows.c) \
    pubnub_sync$(LIB_EXT)
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