/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_version.h"

#include "pubnub_version_internal.h"

#define PUBNUB_SDK_NAME "Microchip.Harmony"


char const *pubnub_sdk_name(void)
{
    return PUBNUB_SDK_NAME;
}


char const *pubnub_version(void)
{
    return PUBNUB_SDK_VERSION;
}


char const *pubnub_uname(void)
{
    return PUBNUB_SDK_NAME "/" PUBNUB_SDK_VERSION;
}

