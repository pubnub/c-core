PROJECT_SOURCEFILES = pubnub_pubsubapi.c pubnub_coreapi.c pubnub_ccore_pubsub.c pubnub_ccore.c pubnub_netcore.c pubnub_alloc_static.c pubnub_assert_std.c pubnub_json_parse.c pubnub_keep_alive.c pubnub_helper.c pubnub_url_encode.c

all: pubnub_proxy_unittest pubnub_timer_list_unittest unittest

OS := $(shell uname)
# Coverage doesn't seem to work on MacOS for some reason, but, since
# we can get it on Linux, we don't want to spend time figuring it out,
# simply don't do it on MacOS.
ifeq ($(OS),Darwin)
GCOVR=echo
COVERAGE_FLAGS=
else
GCOVR=gcovr
COVERAGE_FLAGS=-fprofile-arcs -ftest-coverage
endif

generate_report: all
	#$(GCOVR) -r . --html --html-details -o coverage.html
	$(GCOVR) -r . --xml -o coverage.xml

ifndef RECEIVE_GZIP_RESPONSE
RECEIVE_GZIP_RESPONSE = 1
endif

ifeq ($(RECEIVE_GZIP_RESPONSE), 1)
PROJECT_SOURCEFILES += ../lib/miniz/miniz_tinfl.c pbgzip_decompress.c
endif


CFLAGS +=-g -D PUBNUB_ADVANCED_KEEP_ALIVE=1 -D PUBNUB_LOG_LEVEL=PUBNUB_LOG_LEVEL_WARNING -D PUBNUB_DYNAMIC_REPLY_BUFFER=1 -D PUBNUB_RECEIVE_GZIP_RESPONSE=$(RECEIVE_GZIP_RESPONSE) -I. -I../ -I test -I../lib/base64 -I../lib/md5 -I../lib/miniz -I../cgreen/include

LDFLAGS=-L../cgreen/build/src

CGREEN_RUNNER=../cgreen/build/tools/cgreen-runner
unittest: $(PROJECT_SOURCEFILES) pubnub_core_unit_test.c
	gcc -o pubnub_core_unit_test.so -shared $(CFLAGS) $(LDFLAGS) -D PUBNUB_ORIGIN_SETTABLE=1 -Wall $(COVERAGE_FLAGS) -fPIC $(PROJECT_SOURCEFILES) pubnub_core_unit_test.c -lcgreen -lm
#	gcc -o pubnub_core_unit_testo  $(CFLAGS) -Wall $(COVERAGE_FLAGS) $(PROJECT_SOURCEFILES) pubnub_core_unit_test.c -lcgreen -lm
	$(CGREEN_RUNNER) ./pubnub_core_unit_test.so
#	$(GCOVR) -r . --html --html-details -o coverage.html

TIMER_LIST_SOURCEFILES = pubnub_alloc_static.c pubnub_assert_std.c pubnub_timers.c

pubnub_timer_list_unittest: pubnub_timer_list.c pubnub_timer_list_unit_test.c
	gcc -o pubnub_timer_list_unit_test.so -shared $(CFLAGS) $(LDFLAGS) -D PUBNUB_CALLBACK_API -D PUBNUB_ASSERT_LEVEL_NONE -Wall $(COVERAGE_FLAGS) -fPIC $(TIMER_LIST_SOURCEFILES) pubnub_timer_list.c pubnub_timer_list_unit_test.c -lcgreen -lm
#	gcc -o pubnub_timer_list_unit_testo  $(CFLAGS) -D PUBNUB_CALLBACK_API -Wall $(COVERAGE_FLAGS) $(TIMER_LIST_SOURCEFILES) pubnub_timer_list.c pubnub_timer_list_unit_test.c -lcgreen -lm
	$(CGREEN_RUNNER) ./pubnub_timer_list_unit_test.so
	#$(GCOVR) -r . --html --html-details -o coverage.html

PROXY_PROJECT_SOURCEFILES = pubnub_proxy_core.c pubnub_proxy.c pbhttp_digest.c pbntlm_core.c pbntlm_packer_std.c pubnub_generate_uuid_v4_random_std.c ../lib/pubnub_parse_ipv4_addr.c ../lib/base64/pbbase64.c ../lib/md5/md5.c

pubnub_proxy_unittest: $(PROJECT_SOURCEFILES) $(PROXY_PROJECT_SOURCEFILES) pubnub_proxy_unit_test.c
	gcc -o pubnub_proxy_unit_test.so -shared $(CFLAGS) $(LDFLAGS) -D PUBNUB_PROXY_API=1 -Wall $(COVERAGE_FLAGS) -fPIC $(PROJECT_SOURCEFILES) $(PROXY_PROJECT_SOURCEFILES) pubnub_proxy_unit_test.c -lcgreen -lm
	$(CGREEN_RUNNER) ./pubnub_proxy_unit_test.so
	#$(GCOVR) -r . --html --html-details -o coverage.html

clean:
	rm pubnub_core_unit_test.so pubnub_timer_list_unit_test.so pubnub_proxy_unit_test.so *.gcda *.gcno *.html
