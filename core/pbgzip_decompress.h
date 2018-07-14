#if !defined INC_PUBNUB_DECOMPRESSION
#define	INC_PUBNUB_DECOMPRESSION

#include "pubnub_api_types.h"

/* Types of compressed data format */
enum pubnub_data_compressionType{
    compressionNONE,
    compressionGZIP
};

/** Decompresses(inflates) gzip-formatted data stored in the reply context buffer.
    After decompression puts it back into the same buffer.
    @returns 'PNR_OK' on success,
             'PNR_REPLY_TOO_BIG' lack of memory, or
             'PNR_BAD_COMPRESSION_FORMAT' on failure     
 */
enum pubnub_res pbgzip_decompress(pubnub_t *pb);

#endif /* INC_PUBNUB_DECOMPRESSION */
