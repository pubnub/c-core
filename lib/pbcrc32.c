/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if PUBNUB_USE_GZIP_COMPRESSION

#include "lib/pbcrc32.h"
#include "core/pubnub_assert.h"

static uint32_t crc32_for_byte(uint32_t r)
{
    int j;
    for(j = 0; j < 8; ++j) {
        r = ((r & 1) ? 0 : (uint32_t)0xEDB88320L) ^ (r >> 1);
    }
    return r ^ (uint32_t)0xFF000000L;
}

uint32_t pbcrc32(const void *data, size_t n_bytes)
{
    static uint32_t table[0x100];
    uint32_t crc = 0;
    uint8_t *p, *end;

    PUBNUB_ASSERT_OPT(data != NULL);
    PUBNUB_ASSERT_OPT(n_bytes > 0);
    if (!(*table)) {
        size_t i;
        for(i = 0; i < 0x100; ++i) {
            table[i] = crc32_for_byte(i);
        }
    }
    p   = (uint8_t*)data;
    end = p + n_bytes;
    for (; p < end; ++p) {
        crc = table[(uint8_t)crc ^ (*p)] ^ (crc >> 8);
    }
    return crc;
}

#endif /* PUBNUB_USE_GZIP_COMPRESSION */

