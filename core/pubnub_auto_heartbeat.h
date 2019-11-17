/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PBAUTO_HEARTBEAT
#define INC_PBAUTO_HEARTBEAT

#if PUBNUB_USE_AUTO_HEARTBEAT
/** Maximum number of auto heartbeat thumpers that can be used at one time */
#define PUBNUB_MAX_HEARTBEAT_THUMPERS 16
#define UNASSIGNED PUBNUB_MAX_HEARTBEAT_THUMPERS


/** Pubnub context fields for saving subscribed channels and channel groups
  */
#define M_channelInfo()                                       \
    struct {                                                  \
        char* channel;                                        \
        char* channel_group;                                  \
    } channelInfo;

/** Pubnub context fields for heartbeat info used by the module for keeping presence.
 */
#define M_heartbeatInfo() unsigned thumperIndex;

/** Enables periodical heartbeats that keep presence on subscribed channels and channel
    groups for uuid provided in @p pb context and sets chosen heartbeat period.
    Initially auto heartbeat on @p pb context is disabled.

    This module keeps presence by performing pubnub_heartbeat() transaction periodicaly
    with uuid given whenever subscription on @p pb context is not in progress and
    auto heartbeat is enabled. This process is independent from anything user
    may be doing with the context when its not subscribing(, or heartbeating).

    If the uuid(or any other relevant data, like dns server, or proxy) is changed at
    some point, the module will update it automatically in its heartbeats.
    The same goes if the uuid leaves some of the channels, or channel groups.

    @param pb The pubnub context. Can't be NULL
    @param period_sec Auto heartbeat thumping period in seconds
    @return 0 on success, -1 otherwise
  */
int pubnub_enable_auto_heartbeat(pubnub_t* pb, size_t period_sec);

/** Changes auto heartbeat thumping period. If auto heartbeat is desabled on
    the @p pb context the period wan't be changed and function returns error.
    @param pb The pubnub context. Can't be NULL
    @param period_sec Auto heartbeat thumping period in seconds
    @return 0 on success, -1 otherwise
  */
int pubnub_set_heartbeat_period(pubnub_t* pb, size_t period_sec);

/** Disables auto heartbeat on the @p pb context.
  */
void pubnub_disable_auto_heartbeat(pubnub_t* pb);

/** Returns if auto heartbeat is enabled on the @p pb context.
  */
bool pubnub_is_auto_heartbeat_enabled(pubnub_t* pb);

/** Releases all allocated heartbeat thumpers.
  */
void pubnub_heartbeat_free_thumpers(void);

/** Reads channel and channel groups saved(subscribed on)
  */
void pbauto_heartbeat_read_channelInfo(pubnub_t const* pb,
                                       char const** channel,
                                       char const** channel_group);

/** Gives notice to auto heartbeat module that subscribe, or heartbeat transaction has begun */
void pbauto_heartbeat_transaction_ongoing(pubnub_t const* pb);

/** Starts auto heartbeat timer, if auto heartbeat is enabled, when subscribe transaction
    is finished.
  */
void pbauto_heartbeat_start_timer(pubnub_t const* pb);

/** Releases allocated strings for subscribed channels and channel groups
  */
void pbauto_heartbeat_free_channelInfo(pubnub_t* pb);

/** Preparess channels and channel groups, to be used in request url.
  */
enum pubnub_res pbauto_heartbeat_prepare_channels_and_ch_groups(pubnub_t* pb,
                                                                char const** channel,
                                                                char const** channel_group);

/** Stops auto heartbeat thread */
void pbauto_heartbeat_stop(void);

#else
#define M_channelInfo()
#define M_heartbeatInfo()
#define pubnub_enable_auto_heartbeat(pb, period_sec) -1
#define pubnub_set_heartbeat_period(pb, period_sec) -1
#define pubnub_disable_auto_heartbeat(pb)
#define pubnub_is_auto_heartbeat_enabled(pb) false
#define pubnub_heartbeat_free_thumpers()
#define pbauto_heartbeat_read_channelInfo(pb, channel, channel_group)
#define pbauto_heartbeat_transaction_ongoing(pb)
#define pbauto_heartbeat_start_timer(pb)
#define pbauto_heartbeat_free_channelInfo(pb)
#define pbauto_heartbeat_prepare_channels_and_ch_groups(pb, addr_channel, addr_channel_group) PNR_OK
#define pbauto_heartbeat_stop()
#endif /* PUBNUB_USE_AUTO_HEARTBEAT */

#endif /* !defined INC_PBAUTO_HEARTBEAT */

