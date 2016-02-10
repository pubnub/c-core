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

    The exact behavior when using non-blocking I/O depends on the
    platform, but, in general:

    - getting (or trying to get) the outcome of a Pubnub transaction
    will not block the caller's thread

    - if outcome is gotten by polling (calling a Punbub SDK API to get
    the outcome), each poll will indicate whether the outcome is
    reached or not, so user will have to "call until the outcome is
    reached", though the user, is, of course, free to do other things
    between two "poll calls"

    - if outcome is gotten via a callback or similar means, it is most
    likely that the actual I/O is done non-blocking anyway, but, in
    any case, user would probably see little difference between
    blocking and non-blocking I/O

    In general, non-blocking I/O gives to more complex code, but that
    scales better.

    @pre Call this after pubnub_init() on the context
    @param p The Context to set non-blocking I/O  for

    @return 0: OK, otherwise: error, non-blocking I/O not supported
    
*/
int pubnub_set_non_blocking_io(pubnub_t *p);


/** Sets the usage of blocking I/O for a context. If blocking
    I/O is supported by a platform, it will be used, unless some other
    reason prevents it. 

    The exact behavior when using blocking I/O depends on the
    platform, but, in general:

    - getting (or trying to get) the outcome of a Pubnub transaction
    will block the caller's thread until the outcome is actually
    reached

    - if outcome is gotten by polling (calling a Punbub SDK API to get
    the outcome), the user will call just once and the poll will
    return when the outcome is reached (making it impossible for the
    caller to do anything on that thread until the outcome is reached)

    - if outcome is gotten via a callback or similar means, it is most
    likely that the actual I/O is done non-blocking, but, in any case,
    user would probably see little difference between blocking and
    non-blocking I/O

    In general, blocking I/O gives to simpler code, but that scales
    poorly.

    @pre Call this after pubnub_init() on the context
    @param p The Context to set blocking I/O  for

    @return 0: OK, otherwise: error, blocking I/O not supported
    
*/
int  pubnub_set_blocking_io(pubnub_t *p);



#endif /* defined INC_PUBNUB_BLOCKING_IO */
