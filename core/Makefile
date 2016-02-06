
PROJECT_SOURCEFILES = pubnub_coreapi.c pubnub_ccore.c pubnub_netcore.c pubnub_alloc_static.c pubnub_assert_std.c pubnub_json_parse.c

all: unittest pubnub_timer_list_unittest

CFLAGS +=-g -D PUBNUB_LOG_LEVEL=PUBNUB_LOG_LEVEL_TRACE -D PUBNUB_DYNAMIC_REPLY_BUFFER=1  -I. -I test

unittest: $(PROJECT_SOURCEFILES) pubnub_core_unit_test.c
	gcc -o pubnub_core_unit_test.so -shared $(CFLAGS) -Wall -fprofile-arcs -ftest-coverage -fPIC $(PROJECT_SOURCEFILES) pubnub_core_unit_test.c -lcgreen -lm
#	gcc -o pubnub_core_unit_testo  $(CFLAGS) -Wall -fprofile-arcs -ftest-coverage $(PROJECT_SOURCEFILES) pubnub_core_unit_test.c -lcgreen -lm
	valgrind --quiet cgreen-runner ./pubnub_core_unit_test.so
	gcovr -r . --html --html-details -o coverage.html

TIMER_LIST_SOURCEFILES = pubnub_alloc_static.c pubnub_assert_std.c pubnub_timers.c

pubnub_timer_list_unittest: pubnub_timer_list.c pubnub_timer_list_unit_test.c
	gcc -o pubnub_timer_list_unit_test.so -shared $(CFLAGS) -D PUBNUB_CALLBACK_API -D PUBNUB_ASSERT_LEVEL_NONE -Wall -fprofile-arcs -ftest-coverage -fPIC $(TIMER_LIST_SOURCEFILES) pubnub_timer_list.c pubnub_timer_list_unit_test.c -lcgreen -lm
#	gcc -o pubnub_timer_list_unit_testo  $(CFLAGS) -D PUBNUB_CALLBACK_API -Wall -fprofile-arcs -ftest-coverage $(TIMER_LIST_SOURCEFILES) pubnub_timer_list.c pubnub_timer_list_unit_test.c -lcgreen -lm
	valgrind --quiet cgreen-runner ./pubnub_timer_list_unit_test.so
	gcovr -r . --html --html-details -o coverage.html

clean:
	rm pubnub_core_unit_test.so pubnub_timer_list_unit_test.so
