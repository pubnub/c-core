#include "pubnub_api_types.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

void pbpal_init(pubnub_t* pb)
{
}


int pbpal_send(pubnub_t* pb, void const* data, size_t n)
{
    return 0;
}


int pbpal_send_str(pubnub_t* pb, char const* s)
{
    return pbpal_send(pb, s, strlen(s));
}


enum pubnub_res pbpal_handle_socket_condition(int result, pubnub_t* pb, char const* file, int line)
{
    return 0;
}


int pbpal_send_status(pubnub_t* pb)
{
    return 0;
}


int pbpal_start_read_line(pubnub_t* pb)
{
    return 0;
}


enum pubnub_res pbpal_line_read_status(pubnub_t* pb)
{
    return PNR_OK;
}


int pbpal_read_len(pubnub_t* pb)
{
    return (char*)pb->ptr - pb->core.http_buf;
}


int pbpal_start_read(pubnub_t* pb, size_t n)
{
    return 0;
}


enum pubnub_res pbpal_read_status(pubnub_t* pb)
{
}


bool pbpal_closed(pubnub_t* pb)
{
    return (pb->pal.ssl == NULL) && (pb->pal.socket == SOCKET_INVALID);
}


void pbpal_forget(pubnub_t* pb)
{
    /* a no-op under OpenSSL */
}


int pbpal_close(pubnub_t* pb)
{
    return 0;
}


void pbpal_free(pubnub_t* pb)
{
}
