# Pubnub client library for Contiki OS

Pubnub library for Contiki OS is designed for embedded/constrained
devices. It consists of just two source files (and their header
files). The public (user) interface (API) of the library is
fully documented in Doxygen compatible comments.

There are no special requirements of the library, and it should be
usable as-is on any platform that Contiki is ported to.

It has only the basic operations: publish, subscribe and "leave",
and is designed with minimal amount of code in mind. It's data memory
requirements can be tweaked by the user, but are by design static
and brought down to minimum.


## File Organization

The files of the library repository are:

- `pubnub.h` : the public interface of the library, #include this in
  your code, and search the comments for documentation. You can also
  generate Doxygen documentation from it.

- `pubnub.c` : the implementation of the library, compile &link this
  with your code

- `pubnub_ccore.c` and `pubnub_ccore.h` : internal module. Compile and
  link the source (`.c`) with your code and "forget" about the header
  (`.h`).

- `pubnubDemo.c` : A simple demo of how the library should be used.
  Build this (with pubnub.c and Contiki) for a basic example of how
  stuff works.

- `pubnub.t.c` : The unit test. It uses the cgreen unit testing
  library and has "total" line coverage (only the lines that _can't_
  be executed - sanity checks - are not covered). You can look at it
  to see various ways to interact with the library, but as most test
  code, it's not very user-friendly. You can also build and run it
  yourself, if you wish.

- `Makefile` : basic Makefile to build the pubnubDemo "app" and
  pubnub.t unit test. Use are is, or look for clues on how to make one
  for yourself.

- `LICENSE` and this `README.md` should be self-explanatory.
  
  
## Design considerations

The fundamental flow for working with the library is this:

0. Foremost, you should have a Contiki OS process to handle the
   outcome of your requests. You can work without them, but that would
   be somewhat clumsy. Of course, this can be done in an already
   existing Contiki OS process of yours.
   
1. Obtain a Pubnub "context" from the library. It is an opaque pointer.
   *Note*: you can't create contexts on your own. "All Pubnub contexz
   are belong to us." :)

2. Initialize the context, giving the subscribe and publish key.

3. Start a operation/transaction/Pubnub API call on the context.  It
   will either fail or return an indication of "started". The outcome
   will be sent to your process, with event indicating the transaction
   type and data being the context on which the transaction was
   carried out.

4. On receipt of the process event, check the context for the result
   (success or indication of failure). If OK and it was a subscribe
   transaction, you can get the messages that were fetched (and, if
   available, the channels they pertain too).

5. When you're done processing the outcome event, you can start a new
   transaction in that context.

This is ilustrated in pubnubDemo.c, similar to this:

    static pubnub_t *m_pb = pubnub_get_ctx(0);
    pubnub_init(m_pb, pubkey, subkey);
    
    etimer_set(&et, 3*CLOCK_SECOND);
    
    while (1) {
		PROCESS_WAIT_EVENT();
		if (ev == PROCESS_EVENT_TIMER) {
			pubnub_publish(m_pb, channel, "\"ConTiki Pubnub voyager\"");
		}
		else if (ev == pubnub_publish_event) {
			pubnub_subscribe(m_pb, channel);
		}
		else if (ev == pubnub_subscribe_event) {
			for (;;) {
				char const *msg = pubnub_get(m_pb);
				if (NULL == msg) {
					break;
				}
				printf("Received message: %s\n", msg);
			}
			etimer_restart(&et);
		}
    }

### Remarks

* We said there are no requirements, because we assume the
  minimal Contiki OS. But, in fact, *we do* expect that:

	- you have both UDP and TCP enabled
	- you have IPv4 enabled
	- you have DNS enabled

* While you may have parallel transactions running in different
  contexts, a single context can handle only one transaction at a
  time.

* You have to start the `pubnub_process` at some point, preferrably
  before you initiate any transaction.  It does not start
  automatically.  This enables you to start it at your own
  convinience. The pubnubDemo starts it in the `AUTOSTART_PROCESSES`
  list, but you don't have to do it like that.
