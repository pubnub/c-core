# Compiler and linker flags for POSIX system
# Description: Makefile used to declare compiler and linker flags adjustable
#              with environment variables.


###############################################################################
#                               Compiler flags                                #
###############################################################################

CMAKE_FLAGS = -Wall

# Enable address sanitizer if required.
ifeq ($(ASAN),1)
    # -g                 - enables debugging but with price of larger binary
    # -fsanitize=address - run address sanityzer
    CMAKE_FLAGS += -g -fsanitize=address
endif

ifeq ($(WITH_CPP), 1)
    COMPILER_FLAGS = $(CMAKE_FLAGS) -x c++ $(USER_CXX_FLAGS)
else
    COMPILER_FLAGS = $(CMAKE_FLAGS) $(USER_C_FLAGS)
endif


###############################################################################
#                                Linker flags                                 #
###############################################################################

ifeq ($(shell uname),Darwin)
    LDLIBS = -lpthread
else
    LDLIBS = -lrt -lpthread
endif

ifeq ($(OPENSSL),1)
    ifneq ($(and $(OPENSSL_ROOT_DIR),$(CUSTOM_OPENSSL_LIB_DIR)),)
        LDLIBS += -L$(OPENSSL_ROOT_DIR)/$(CUSTOM_OPENSSL_LIB_DIR)
    else ifeq ($(shell uname),Darwin)
        # Set OpenSSL lib path.
        OPENSSL_LIBS := $(shell pkg-config --libs openssl 2>/dev/null)
        ifneq ($(OPENSSL_LIBS),)
            LDLIBS += $(OPENSSL_LIBS)
        endif
    endif
    LDLIBS += -lssl -lcrypto
endif

LDLIBS := $(sort $(LDLIBS))