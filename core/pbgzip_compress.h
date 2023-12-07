/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if PUBNUB_USE_GZIP_COMPRESSION

#if !defined INC_PUBNUB_COMPRESSION
#define INC_PUBNUB_COMPRESSION

#include "pubnub_api_types.h"

/** Compresses(deflates) @p message into gzip-formatted data stored in context buffer.
    @retval PNR_OK on success,
    @retval PNR_STARTED on poor comression ratio,
    @retval PNR_BAD_COMPRESSION_FORMAT on error     
 */
enum pubnub_res pbgzip_compress(pubnub_t *pb, char const* message);

#endif /* INC_PUBNUB_COMPRESSION */

#endif /* PUBNUB_USE_GZIP_COMPRESSION */
