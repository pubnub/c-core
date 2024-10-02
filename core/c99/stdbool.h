#pragma once

#if !defined __cplusplus

#define false   0
#define true    1

#ifdef PUBNUB_BOOL_TYPE
#define bool PUBNUB_BOOL_TYPE
#else
#define bool int
#endif

#endif /* !defined __cplusplus */
