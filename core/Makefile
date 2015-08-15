PROJECT_SOURCEFILES = pubnub_coreapi.c pubnub_ccore.c pubnub_netcore.c pubnub_alloc_static.c pubnub_assert_std.c pubnub_json_parse.c

all: unittest

CFLAGS +=-g -D VERBOSE_DEBUG -I. -I test

unittest: $(PROJECT_SOURCEFILES) pubnub_core_unit_test.c
	gcc -o pubnub_core_unit_test.so -shared $(CFLAGS) -Wall -fprofile-arcs -ftest-coverage -fPIC $(PROJECT_SOURCEFILES) pubnub_core_unit_test.c -lcgreen -lm
	valgrind --quiet cgreen-runner ./pubnub_core_unit_test.so
	#cgreen-runner  ./pubnub_core_unit_test.so
	gcovr -r . --html --html-details -o coverage.html
