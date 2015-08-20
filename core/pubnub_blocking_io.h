/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_BLOCKING_IO
#define	INC_PUBNUB_BLOCKING_IO


#include "pubnub_api_types.h"

#include <stdbool.h>


/** @file pubnub_blocking_io.h 
    This is the "Blocking I/O" API of the Pubnub client library.
    Functions here influence the way that Pubnub client library
    works with lower levels (the TCP/IP stack).

    It is available on platforms that support setting blocking /
    non-blocking behavior. Be aware that, even on such platforms,
    most of these functions provide more of a hint, rather than
    a command.
*/


/** Sets the usage of non-blocking I/O for a context. If non-blocking
    I/O is supported by a platform, it will be used, unless some other
    reason prevents it. 

    @pre Call this after pubnub_init() on the context
    @param p The Context to set non-blocking I/O  for

    @return 0: OK, otherwise: error, blocking I/O not supported
    
*/
int pubnub_set_non_blocking_io(pubnub_t *p);


/** Sets the usage of blocking I/O for a context. If blocking
    I/O is supported by a platform, it will be used, unless some other
    reason prevents it. 

    @pre Call this after pubnub_init() on the context
    @param p The Context to set non-blocking I/O  for

    @return 0: OK, otherwise: error, blocking I/O not supported
    
*/
int  pubnub_set_blocking_io(pubnub_t *p);



#endif /* defined INC_PUBNUB_BLOCKING_IO */
