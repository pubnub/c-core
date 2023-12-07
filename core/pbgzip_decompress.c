/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if PUBNUB_RECEIVE_GZIP_RESPONSE

#include "pubnub_internal.h"

#include "core/pubnub_assert.h"
#include "lib/miniz/miniz_tinfl.h"
#include "core/pubnub_log.h"

#define GZIP_HEADER_LENGTH_BYTES 10
#define GZIP_FOOTER_LENGTH_BYTES 8

static enum pubnub_res inflate_total_to_context_buffer(pubnub_t*      pb,
                                                       uint8_t const* p_in_buf_next,
                                                       size_t in_buf_size,
                                                       size_t out_len)
{
    tinfl_decompressor decomp;
    size_t             dst_buf_size = out_len;
    tinfl_init(&decomp);
    tinfl_status status =
        tinfl_decompress(&decomp,
                         (const mz_uint8*)p_in_buf_next,
                         &in_buf_size,
                         (mz_uint8*)pb->core.decomp_http_reply,
                         (mz_uint8*)pb->core.decomp_http_reply,
                         &dst_buf_size,
                         TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
    switch (status) {
    case TINFL_STATUS_DONE:
        if (dst_buf_size == out_len) {
            return PNR_OK;
        }
        else {
            PUBNUB_LOG_ERROR("Decompressed length[%u] is smaller than the "
                             "'unpacked_size' value[%u]!\n"
                             "(Unpacked:['%.*s'])\n",
                             (unsigned)dst_buf_size,
                             (unsigned)out_len,
                             (int)dst_buf_size,
                             pb->core.decomp_http_reply);
        }
        break;
    case TINFL_STATUS_FAILED_CANNOT_MAKE_PROGRESS:
        PUBNUB_LOG_ERROR(
            "'Tinfl'-decompress status: failed(cannot make progress)!\n");
        break;
    case TINFL_STATUS_FAILED:
        PUBNUB_LOG_ERROR("'Tinfl'-decompress status: failed!\n");
        break;
    case TINFL_STATUS_HAS_MORE_OUTPUT:
        PUBNUB_LOG_ERROR("Decompressed length is grater than the "
                         "'unpacked_size' value[%u]!\n"
                         "(Unpacked:['%.*s'])\n",
                         (unsigned)out_len,
                         (int)out_len,
                         pb->core.decomp_http_reply);
        break;
    default:
        if (status < 0) {
            PUBNUB_LOG_ERROR("Decompression failed(Status: %d)!", status);
        }
        else {
            PUBNUB_LOG_ERROR("'Tinfl'-decompress status: %d!\n", status);
        }
        break;
    }
    return PNR_BAD_COMPRESSION_FORMAT;
}

#if !PUBNUB_DYNAMIC_REPLY_BUFFER
PUBNUB_STATIC_ASSERT(sizeof((pubnub_t*)0)->core.http_reply == sizeof((pubnub_t*)0)->core.decomp_http_reply,
                     http_reply_and_gzip_decompression_buffer_dont_match);
#endif

static void swap_reply_buffer(pubnub_t* pb)
{
#if PUBNUB_DYNAMIC_REPLY_BUFFER
    char*  aux_buf             = pb->core.http_reply;
    size_t aux_buf_len         = pb->core.http_buf_len;
    pb->core.http_reply        = pb->core.decomp_http_reply;
    pb->core.http_buf_len      = pb->core.decomp_buf_size;
    pb->core.decomp_http_reply = aux_buf;
    pb->core.decomp_buf_size   = aux_buf_len;
#else
    PUBNUB_ASSERT(pb->core.decomp_buf_size < sizeof pb->core.decomp_http_reply);
    memcpy(pb->core.http_reply, pb->core.decomp_http_reply, pb->core.decomp_buf_size);
    pb->core.http_buf_len = pb->core.decomp_buf_size;
#endif /* PUBNUB_DYNAMIC_REPLY_BUFFER */
    return;
}


static enum pubnub_res inflate_total(pubnub_t*      pb,
                                     const uint8_t* p_in_buf_next,
                                     size_t         in_buf_size,
                                     size_t         out_len)
{
    enum pubnub_res result;
#if PUBNUB_DYNAMIC_REPLY_BUFFER
    if (pb->core.decomp_buf_size < out_len) {
        char* newbuf = (char*)realloc(pb->core.decomp_http_reply, out_len + 1);
        if (NULL == newbuf) {
            PUBNUB_LOG_ERROR("Failed to reallocate decompression buffer!\n"
                             "Out length:%lu\n",
                             (unsigned long)out_len);
            return PNR_REPLY_TOO_BIG;
        }
        pb->core.decomp_http_reply = newbuf;
    }
#else
    if (out_len >= sizeof pb->core.decomp_http_reply) {
        PUBNUB_LOG_ERROR("Decompression buffer too small!\n"
                         "Size of buffer:%lu - Out length:%lu\n",
                         (unsigned long)sizeof pb->core.decomp_http_reply,
                         (unsigned long)out_len);
        return PNR_REPLY_TOO_BIG;
    }
#endif
    result =
        inflate_total_to_context_buffer(pb, p_in_buf_next, in_buf_size, out_len);
    if (result == PNR_OK) {
        pb->core.decomp_buf_size = out_len;
        swap_reply_buffer(pb);
    }
    return result;
}


enum pubnub_res pbgzip_decompress(pubnub_t* pb)
{
    const uint8_t* data = (uint8_t*)pb->core.http_reply;
    size_t         size = (size_t)pb->core.http_buf_len;
    uint32_t       unpacked_size;

    if ((size < (GZIP_HEADER_LENGTH_BYTES + GZIP_FOOTER_LENGTH_BYTES))
        || (data[0] != 0x1f) || (data[1] != 0x8b)) {
        PUBNUB_LOG_ERROR("Compressed data format is not gzip!\n");
        return PNR_BAD_COMPRESSION_FORMAT;
    }
    if (data[2] != 8) {
        PUBNUB_LOG_ERROR("Unknown compression method %uX - only 'deflate'(8) "
                         "is supported!\n",
                         (unsigned)data[2]);
        return PNR_BAD_COMPRESSION_FORMAT;
    }
    if (data[3] != 0) {
        PUBNUB_LOG_ERROR("GZIP flags should be 0, but are %uX\n",
                         (unsigned)data[3]);
        return PNR_BAD_COMPRESSION_FORMAT;
    }
    /* Unpacked message size is placed at the end of the 'gzip' formated message
       in the last four bytes
     */
    unpacked_size = (uint32_t)data[size - 4];
    unpacked_size |= (uint32_t)data[size - 3] << 8;
    unpacked_size |= (uint32_t)data[size - 2] << 16;
    unpacked_size |= (uint32_t)data[size - 1] << 24;
    PUBNUB_LOG_TRACE("pbgzip_decompress(pb=%p)-Length before:%lu and after "
                     "decompresion:%lu\n",
                     pb,
                     (unsigned long)size,
                     (unsigned long)unpacked_size);
    size -= (GZIP_HEADER_LENGTH_BYTES + GZIP_FOOTER_LENGTH_BYTES);

    return inflate_total(
        pb, data + GZIP_HEADER_LENGTH_BYTES, size, (size_t)unpacked_size);
}

#endif /* PUBNUB_RECEIVE_GZIP_RESPONSE */

