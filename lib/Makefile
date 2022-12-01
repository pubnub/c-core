
all: pubnub_parse_ipv6_addr_unit_test pubnub_dns_codec_unit_test

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

CFLAGS +=-g -D PUBNUB_CALLBACK_API -D PUBNUB_LOG_LEVEL=PUBNUB_LOG_LEVEL_NONE -I. -I.. -I../core -I ../core/test -I../lib/base64 -I../lib/md5 -I../lib/miniz -I../cgreen/include

LDFLAGS=-L../cgreen/build/src

CGREEN_RUNNER=../cgreen/build/tools/cgreen-runner

PARSE_IPV6_ADDR_SOURCE_FILES = ../core/pubnub_assert_std.c

pubnub_parse_ipv6_addr_unit_test: pubnub_parse_ipv6_addr.c pubnub_parse_ipv6_addr_unit_test.c
	gcc -o pubnub_parse_ipv6_addr_unit_test.so -shared $(CFLAGS) $(LDFLAGS) -Wall $(COVERAGE_FLAGS) -fPIC $(PARSE_IPV6_ADDR_SOURCE_FILES) pubnub_parse_ipv6_addr.c pubnub_parse_ipv6_addr_unit_test.c -lcgreen -lm
#	gcc -o pubnub_parse_ipv6_addr_unit_testo $(CFLAGS) -Wall $(COVERAGE_FLAGS) $(PARSE_IPV6_ADDR_SOURCE_FILES) ../lib/pubnub_parse_ipv6_addr.c pubnub_parse_ipv6_addr_unit_test.c -lcgreen -lm
	$(CGREEN_RUNNER) ./pubnub_parse_ipv6_addr_unit_test.so
	$(GCOVR) -r . --html --html-details -o coverage.html

DNS_CODEC_SOURCE_FILES = ../core/pubnub_assert_std.c

pubnub_dns_codec_unit_test: pubnub_dns_codec.c pubnub_dns_codec_unit_test.c
	gcc -o pubnub_dns_codec_unit_test.so -shared $(CFLAGS) -D PUBNUB_USE_IPV6=1 -D PUBNUB_USE_SSL=1 $(LDFLAGS) -Wall $(COVERAGE_FLAGS) -fPIC $(DNS_CODEC_SOURCE_FILES) pubnub_dns_codec.c pubnub_dns_codec_unit_test.c -lcgreen -lm
#	gcc -o pubnub_dns_codec_unit_testo $(CFLAGS) -Wall $(COVERAGE_FLAGS) $(DNS_CODEC_SOURCE_FILES) ../lib/pubnub_dns_codec.c pubnub_dns_codec_unit_test.c -lcgreen -lm
	$(CGREEN_RUNNER) ./pubnub_dns_codec_unit_test.so
	$(GCOVR) -r . --html --html-details -o coverage.html

clean:
	find . -type d -iname "*.dSYM" -exec rm -rf {} \+
	find . -type f -name "*.so" -o -name "*.gcda" -o -name "*.gcno" -o -name "*.html" | xargs -r rm -rf
