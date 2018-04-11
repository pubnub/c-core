/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PBPAL_ADD_SYSTEM_CERTS
#define      INC_PBPAL_ADD_SYSTEM_CERTS

#include "core/pubnub_api_types.h"

/** Adds CA certificates from the system store. Available on platforms
    that have a system store (like Windows).
 */
int pbpal_add_system_certs(pubnub_t* pb);


#endif /* !defined INC_PBPAL_ADD_SYSTEM_CERTS */
