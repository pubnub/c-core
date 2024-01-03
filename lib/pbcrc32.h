/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if PUBNUB_USE_GZIP_COMPRESSION

#if !defined INC_PB_CRC32
#define INC_PB_CRC32

#include <stdint.h>
#include <stdlib.h>

/* The standard CRC32 checksum(Cyclic Redundancy Checksum).
   Returns the checksum for (unpacked) @p data with given @p n_bytes length
   in octets.
 */ 
uint32_t pbcrc32(const void *data, size_t n_bytes);

#endif /* INC_PB_CRC32 */

#endif /* PUBNUB_USE_GZIP_COMPRESSION */

