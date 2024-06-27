/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbpal.h"

#include "pubnub_internal.h"
#include "pubnub_log.h"

#include <fcntl.h>


int pbpal_set_blocking_io(pubnub_t *pb)
{
    PUBNUB_LOG_WARNING("pbpal_set_blocking_io() - Unsupported\n");
	return -1;
}
