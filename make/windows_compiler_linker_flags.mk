# Compiler and linker flags for Windows system
# Description: Makefile used to declare compiler and linker flags adjustable
#              with environment variables.


###############################################################################
#                               Compiler flags                                #
###############################################################################

!if $(WITH_GCC)
# -std=c11 - enables `%z` and `%ll` in printf() - in general, we're "low"
#            on C11 features
NMAKE_FLAGS = -std=c11 -W2
!else
# /MP - uses one compiler (`cl`) process for each input file, enabling
#       faster build
NMAKE_FLAGS = /W2 /MP
!endif

!if $(ASAN)
!if $(WITH_GCC)
    # -g                 - enables debugging but with price of larger binary
    # -fsanitize=address - run address sanityzer
    NMAKE_FLAGS = $(NMAKE_FLAGS) -g -fsanitize=address
!else
    # /Zi      - enables debugging, remove to get a smaller .exe and no .pdb
    # /analyze - To run the static analyzer (not compatible w/clang-cl)
    NMAKE_FLAGS = $(NMAKE_FLAGS) /Zi /analyze
!endif
!else
NMAKE_FLAGS = $(NMAKE_FLAGS) /wd4005
!endif

!if $(WITH_CPP)
!if $(WITH_GCC)
    NMAKE_FLAGS = $(NMAKE_FLAGS) -x c++
!else
    NMAKE_FLAGS = $(NMAKE_FLAGS) /TP
!endif
!endif
COMPILER_FLAGS = $(NMAKE_FLAGS) $(USER_C_FLAGS)


###############################################################################
#                                Linker flags                                 #
###############################################################################

!if $(WITH_GCC)
LDLIBS = -lws2_32 -lrpcrt4 -lsecur32
!else
LDLIBS = ws2_32.lib IPHlpAPI.lib rpcrt4.lib
!endif

# TODO: Add support of OpenSSL when built with GCC if possible.
!if "$(OPENSSL)" == "1" && "$(WITH_GCC)" == "0"
OPENSSL_LIBS =
!if "$(OPENSSL_ROOT_DIR)" != "" && "$(CUSTOM_OPENSSL_LIB_DIR)" != ""
    !if EXISTS($(OPENSSL_ROOT_DIR)$(PATH_SEP)$(CUSTOM_OPENSSL_LIB_DIR)$(PATH_SEP)libssl.lib)
        OPENSSL_LIBS = \
            $(OPENSSL_ROOT_DIR)$(PATH_SEP)$(CUSTOM_OPENSSL_LIB_DIR)$(PATH_SEP)libssl.lib    \
            $(OPENSSL_ROOT_DIR)$(PATH_SEP)$(CUSTOM_OPENSSL_LIB_DIR)$(PATH_SEP)libcrypto.lib
    !elseif EXISTS($(OPENSSL_ROOT_DIR)$(PATH_SEP)$(CUSTOM_OPENSSL_LIB_DIR)$(PATH_SEP)ssleay32.lib)
        OPENSSL_LIBS = \
            $(OPENSSL_ROOT_DIR)$(PATH_SEP)$(CUSTOM_OPENSSL_LIB_DIR)$(PATH_SEP)ssleay32.lib \
            $(OPENSSL_ROOT_DIR)$(PATH_SEP)$(CUSTOM_OPENSSL_LIB_DIR)$(PATH_SEP)libeay32.lib
    !endif
!endif
!if "$(OPENSSL_LIBS)" == ""
    !error "Cannot find OpenSSL libraries, OPENSSLPATH=$(OPENSSL_ROOT_DIR)"
!else
    LDLIBS = $(LDLIBS) $(OPENSSL_LIBS)
!endif
!endif