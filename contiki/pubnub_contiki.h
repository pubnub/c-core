/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_CONTIKI
#define	INC_PUBNUB_CONTIKI

/** @mainpage The ConTiki OS Pubnub client library

    This is the Pubnub client library for the ConTiki OS. It is
    carefully designed for small footprint and to be a good fit for
    ConTiki OS way of multitasking with Protothreads and Contiki OS
    processes. You can have multiple pubnub contexts established; in
    each context, at most one Pubnub API call/transaction may be
    ongoing (typically a "publish" or a "subscribe").

    It has less features than most Pubnub libraries for other OSes, as
    it is designed to be used in more constrained environments.

    The most important differences from a full-fledged Pubnub API
    implementation are:

    - The only available Pubnub APIs are: publish, subscribe, leave.

    - Library itself doesn't handle timeouts other than TCP/IP
    timeout, which mostly comes down to loss of connection to the
    server. If you want to impose a timeout on transaction duration,
    use one of the several ConTiki OS timer interfaces yourself.

    - You can't change the origin (URL) or several other parameters of
    connection to Pubnub.
    
 */

/** @file pubnub_contiki.h */

#include "pubnub_config.h"

/* -- You should not change anything below this line -- */

#include "pubnub_alloc.h"
#include "pubnub_assert.h"
#include "pubnub_coreapi.h"
#include "pubnub_ntf_contiki.h"


#endif /* !defined INC_PUBNUB_CONTIKI */
