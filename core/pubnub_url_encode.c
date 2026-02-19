/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"
#include "pubnub_url_encode.h"

#include "pubnub_assert.h"
#include "pubnub_api_types.h"
#if PUBNUB_USE_LOGGER
#include "pbcc_logger_manager.h"
#endif // PUBNUB_USE_LOGGER

#include <string.h>


#if PUBNUB_LOG_ENABLED(ERROR)
static void log_buffer_overflow_(struct pbcc_context* pb,
                                 char const*          buffer,
                                 size_t               buffer_size,
                                 char const*          remaining)
{
    if (!pbcc_logger_manager_should_log(pb->logger_manager,
                                        PUBNUB_LOG_LEVEL_ERROR))
        return;

    pubnub_log_value_t data = pubnub_log_value_map_init();
    PUBNUB_LOG_MAP_SET_NUMBER(&data, (double)buffer_size, buffer_size)
    PUBNUB_LOG_MAP_SET_STRING(&data, buffer, encoded_so_far)
    PUBNUB_LOG_MAP_SET_NUMBER(&data, (double)strlen(buffer), encoded_length)
    PUBNUB_LOG_MAP_SET_STRING(&data, remaining, remaining_input)
    PUBNUB_LOG_MAP_SET_NUMBER(&data, (double)strlen(remaining), remaining_length)
    pbcc_logger_manager_log_object(pb->logger_manager,
                                   PUBNUB_LOG_LEVEL_ERROR,
                                   PUBNUB_LOG_LOCATION,
                                   &data,
                                   "URL-encoded string exceeded buffer capacity");
}
#endif // PUBNUB_USE_LOGGER && PUBNUB_LOG_ENABLED(ERROR)


int pubnub_url_encode(struct pbcc_context* pb, char* buffer, char const* what, size_t buffer_size, enum pubnub_trans pt)
{
    char *okSpanChars;
    int i = 0;

    PUBNUB_ASSERT_OPT(buffer != NULL);
    PUBNUB_ASSERT_OPT(what != NULL);

    switch(pt){
#if PUBNUB_USE_OBJECTS_API || PUBNUB_USE_REVOKE_TOKEN_API
#if PUBNUB_USE_OBJECTS_API
    case PBTT_GETALL_UUIDMETADATA :
    case PBTT_SET_UUIDMETADATA:
    case PBTT_GET_UUIDMETADATA:
    case PBTT_GETALL_CHANNELMETADATA:
    case PBTT_SET_CHANNELMETADATA:
    case PBTT_GET_CHANNELMETADATA:
    case PBTT_GET_MEMBERSHIPS:
    case PBTT_SET_MEMBERSHIPS:
    case PBTT_REMOVE_MEMBERSHIPS:
    case PBTT_GET_MEMBERS:
    case PBTT_ADD_MEMBERS:
    case PBTT_SET_MEMBERS:
    case PBTT_REMOVE_MEMBERS:
#if PUBNUB_USE_ADVANCED_HISTORY
    case PBTT_MESSAGE_COUNTS:
#endif // PUBNUB_USE_ADVANCED_HISTORY
#endif // PUBNUB_USE_OBJECTS_API
#if PUBNUB_USE_REVOKE_TOKEN_API
    case PBTT_REVOKE_TOKEN:
#endif // PUBNUB_USE_REVOKE_TOKEN_API
        okSpanChars = OK_SPAN_CHARS_MINUS_COMMA;
        break;
#endif // PUBNUB_USE_OBJECTS_API || PUBNUB_USE_REVOKE_TOKEN_API
    default:
        okSpanChars = OK_SPAN_CHARACTERS;
    }

    *buffer='\0';
    while (what[0]) {
        /* RFC 3986 Unreserved characters plus few
         * safe reserved ones. */
        size_t okspan = strspn(what, okSpanChars);

        if (okspan > 0) {
            if (okspan >= (unsigned)(buffer_size - i - 1)) {
#if PUBNUB_LOG_ENABLED(ERROR)
                log_buffer_overflow_(pb, buffer, buffer_size, what);
#else
                (void)pb;
#endif
                return -1;
            }
            memcpy(buffer + i, what, okspan);
            i += okspan;
            buffer[i] = '\0';
            what += okspan;
        }
        if (what[0]) {
            /* %-encode a non-ok character. */
            char enc[4] = { '%' };
            enc[1]      = "0123456789ABCDEF"[(unsigned char)what[0] / 16];
            enc[2]      = "0123456789ABCDEF"[(unsigned char)what[0] % 16];
            if (3 > buffer_size - i - 1) {
#if PUBNUB_LOG_ENABLED(ERROR)
                log_buffer_overflow_(pb, buffer, buffer_size, what);
#else
                (void)pb;
#endif
                return -1;
            }
            /* Last copied character is '\0' */
            memcpy(buffer + i, enc, 4);
            i += 3;
            ++what;
        }
    }

    return i;
}
