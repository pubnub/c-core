/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "core/pubnub_version.h"

#include "core/pubnub_version_internal.h"


#define PUBNUB_SDK_NAME "Windows"

#ifndef PUBNUB_SDK_VERSION_SUFFIX
#define PUBNUB_SDK_VERSION_SUFFIX
#endif


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
    return PUBNUB_SDK_NAME "-PubNub-C-core/" PUBNUB_SDK_VERSION PUBNUB_SDK_VERSION_SUFFIX;
}


char const *pubnub_uagent(void)
{
    return PUBNUB_SDK_NAME "-PubNub-C-core/" PUBNUB_SDK_VERSION PUBNUB_SDK_VERSION_SUFFIX;
}
