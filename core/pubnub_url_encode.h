/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_URL_ENCODE
#define INC_PUBNUB_URL_ENCODE

/* RFC 3986 Unreserved characters plus few
 * safe reserved ones. */
#define OK_SPAN_CHARACTERS (char*)"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.~,[]"
#define OK_SPAN_CHARS_MINUS_COMMA (char*)"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.~[]"

#include "lib/pb_extern.h"

/** Url-encodes string @p what to user provided @p buffer which has its @p buffer_size.
    @retval length of url-encoded string on success,
    @retval -1 on error(url-encoded string too long for buffer provided)
 */
PUBNUB_EXTERN int pubnub_url_encode(char* buffer, char const* what, size_t buffer_size, enum pubnub_trans pt);

#endif /* !defined INC_PUBNUB_URL_ENCODE */
