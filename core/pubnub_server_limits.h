/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_SERVER_LIMITS
#define      INC_PUBNUB_SERVER_LIMITS

/** Minimal presence heartbeat interval supported by
    Pubnub, in seconds.
*/
#define PUBNUB_MINIMAL_HEARTBEAT_INTERVAL 300
#define PUBNUB_MINIMUM_PRESENCE_HEARTBEAT_VALUE 20
/** Default presence heartbeat value to use if user didn't set any. */
#define PUBNUB_DEFAULT_PRESENCE_HEARTBEAT_VALUE 300

/** Default limit for here_now queries - maximum number of users returned.
    Valid range is 1 to 1000.
*/
#define PUBNUB_DEFAULT_HERE_NOW_LIMIT 1000

/** The maximum channel name length */
#define PUBNUB_MAX_CHANNEL_NAME_LENGTH 92

#endif /* !defined INC_PUBNUB_SERVER_LIMITS */

