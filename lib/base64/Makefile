all: pbbase64_demo

C_FLAGS=-I ../../core

pbbase64_demo: pbbase64_demo.c pbbase64.c ../../core/pubnub_assert_std.c pbbase64.h
	$(CC) -o pbbase64_demo -g $(C_FLAGS) pbbase64_demo.c pbbase64.c ../../core/pubnub_assert_std.c 
