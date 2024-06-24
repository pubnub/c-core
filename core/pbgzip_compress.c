/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if PUBNUB_USE_GZIP_COMPRESSION

#include "pubnub_internal.h"

#include "core/pubnub_assert.h"
#include "lib/miniz/miniz_tdef.h"
#include "lib/pbcrc32.h"
#include "core/pubnub_log.h"

#define GZIP_HEADER_LENGTH_BYTES 10
#define GZIP_FOOTER_LENGTH_BYTES 8
/* Percents 'off' message length after compression */
#define PUBNUB_MINIMAL_ACCEPTABLE_COMPRESSION_RATIO 10

static enum pubnub_res deflate_total_to_context_buffer(pubnub_t*   pb,
                                                       char const* message,
                                                       size_t      message_size)
{
    size_t unpacked_size = message_size;
    size_t compressed = sizeof pb->core.gzip_msg_buf -
                        (GZIP_HEADER_LENGTH_BYTES + GZIP_FOOTER_LENGTH_BYTES);
    char* gzip_msg_buf = pb->core.gzip_msg_buf;
    tdefl_compressor comp;

    tdefl_init(&comp, NULL, NULL, TDEFL_DEFAULT_MAX_PROBES);
    tdefl_status status = tdefl_compress(&comp,
                                         message,
                                         &message_size,
                                         gzip_msg_buf + GZIP_HEADER_LENGTH_BYTES,
                                         &compressed,
                                         TDEFL_FINISH);
    switch (status) {
    case TDEFL_STATUS_DONE:
        if (message_size == unpacked_size) {
            uint32_t crc;
            size_t packed_size = GZIP_HEADER_LENGTH_BYTES + compressed + GZIP_FOOTER_LENGTH_BYTES;
            size_t diff = unpacked_size - packed_size;

            PUBNUB_LOG_TRACE("deflate_total_to_context_buffer(pb=%p) - "
                             "Length before compression: %lu bytes - "
                             "Length after compression: %lu bytes - "
                             "compression ratio=%ld o/oo\n",
                             pb,
                             (unsigned long)unpacked_size,
                             (unsigned long)packed_size,
                             (long)(diff*1000)/(long)unpacked_size);
            if ((packed_size > unpacked_size) ||
                ((diff*100)/unpacked_size < PUBNUB_MINIMAL_ACCEPTABLE_COMPRESSION_RATIO)) {
                /* With insufficient compression we choose not to pack */
                return PNR_STARTED;
            }
            /* Cyclic redundancy checksum data(little endian) */
            crc = pbcrc32(message, unpacked_size);
            gzip_msg_buf[packed_size - 5] = crc >> 24;
            gzip_msg_buf[packed_size - 6] = (crc >> 16) & 0xFF;
            gzip_msg_buf[packed_size - 7] = (crc >> 8) & 0xFF;
            gzip_msg_buf[packed_size - 8] = crc & 0xFF;
            /* Unpacked message size is placed at the end of the 'gzip' formated message
               in the last four bytes(little endian)
             */
            gzip_msg_buf[packed_size - 1] = (uint32_t)unpacked_size >> 24;
            gzip_msg_buf[packed_size - 2] = ((uint32_t)unpacked_size >> 16) & 0xFF;
            gzip_msg_buf[packed_size - 3] = ((uint32_t)unpacked_size >> 8) & 0xFF;
            gzip_msg_buf[packed_size - 4] = (uint32_t)unpacked_size & 0xFF;
            pb->core.gzip_msg_len = packed_size;
            return PNR_OK;
        }
        else {
            PUBNUB_LOG_ERROR("deflate_total_to_context_buffer(pb=%p) - "
                             "Hasn't compressed entire message: %zu bytes compressed,"
                             "unpacked_size=%zu\n",
                             pb,
                             message_size,
                             unpacked_size);
        }
        break;
    case TDEFL_STATUS_BAD_PARAM:
        PUBNUB_LOG_ERROR("deflate_total_to_context_buffer(pb=%p) - "
                         "compression failed : bad parameters\n",
                         pb);
        break;
    case TDEFL_STATUS_OKAY:
    case TDEFL_STATUS_PUT_BUF_FAILED:
        PUBNUB_LOG_ERROR("deflate_total_to_context_buffer(pb=%p) - "
                         "compression failed : buffer to small\n",
                         pb);
        break;
    default:
        if (status < 0) {
            PUBNUB_LOG_ERROR("deflate_total_to_context_buffer(pb=%p) - "
                             "compression failed(status: %d)\n",
                             pb,
                             status);
        }
        else {
            PUBNUB_LOG_ERROR("deflate_total_to_context_buffer(pb=%p) - "
                             "compression status: %d!\n",
                             pb,
                             status);
        }
        break;
    }
    return PNR_BAD_COMPRESSION_FORMAT;
}

/* Compile-time assertion */
PUBNUB_STATIC_ASSERT(sizeof (*(pubnub_t*)(NULL)).core.gzip_msg_buf
                     > (GZIP_HEADER_LENGTH_BYTES + GZIP_FOOTER_LENGTH_BYTES),
                     gzip_msg_buf_too_small_);

enum pubnub_res pbgzip_compress(pubnub_t* pb, char const* message)
{
    char* data;
    size_t size;

    PUBNUB_ASSERT_OPT(pb != NULL);
    PUBNUB_ASSERT_OPT(message != NULL);

    pb->core.gzip_msg_len = 0;
    data = pb->core.gzip_msg_buf;
    /* Gzip format */
    data[0] = 0x1f;
    data[1] = 0x8b; 
    /* Deflate algorithm */
    data[2] = 8;
    /* flags: no file_name, no f_extras, no f_comment, no f_hcrc */
    memset(data + 3, '\0', 7);
    size = strlen(message);
    PUBNUB_LOG_TRACE("pbgzip_compress(pb=%p) - Length before compression:%lu bytes\n", pb, (long unsigned int)size);

    return deflate_total_to_context_buffer(pb, message, size);
}

#endif /* PUBNUB_USE_GZIP_COMPRESSION */
