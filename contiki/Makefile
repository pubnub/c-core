PROJECT_SOURCEFILES += pubnub_coreapi.c pubnub_ccore.c pubnub_netcore.c pubnub_ntf_contiki.c pbpal_contiki.c pubnub_alloc_static.c pubnub_assert_contiki.c
CONTIKI_PROJECT = pubnubDemo
all: test $(CONTIKI_PROJECT)
#all: $(CONTIKI_PROJECT)

#UIP_CONF_IPV6=1
TARGET = minimal-net
CONTIKI = ./contiki-2.7
include $(CONTIKI)/Makefile.include
CFLAGS += -D VERBOSE_DEBUG -D PUBNUB_USE_MDNS=0

unittest: pubnub_coreapi.c pubnub_ccore.c pubnub_netcore.c pubnub_ntf_contiki.c pbpal_contiki.c  pubnub_alloc_static.c pubnub_assert_contiki.c pubnub.t.c
	gcc -o pubnub.t.so -shared $(CFLAGS) -Wall -fprofile-arcs -ftest-coverage -fPIC pubnub_coreapi.c pubnub_ccore.c pubnub_netcore.c pubnub_ntf_contiki.c pbpal_contiki.c pubnub_alloc_static.c pubnub_assert_contiki.c pubnub.t.c -lcgreen -lm
	#valgrind --quiet cgreen-runner ./pubnub.t.so
	cgreen-runner  ./pubnub.t.so

