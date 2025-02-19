/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PBAUTO_HEARTBEAT
#define INC_PBAUTO_HEARTBEAT

#if PUBNUB_USE_AUTO_HEARTBEAT
#if defined      _WIN32
typedef DWORD    pubnub_thread_t;
typedef FILETIME pubnub_timespec_t;
typedef void     pubnub_watcher_t;
#define thread_handle_field() HANDLE thread_handle;
#define UNIT_IN_MILLI 1000
#else
typedef pthread_t       pubnub_thread_t;
typedef struct timespec pubnub_timespec_t;
typedef void*           pubnub_watcher_t;
#define thread_handle_field()
#endif


/** Maximum number of auto heartbeat thumpers that can be used at one time */
#define PUBNUB_MAX_HEARTBEAT_THUMPERS 16
#define UNASSIGNED PUBNUB_MAX_HEARTBEAT_THUMPERS

struct pubnub_heartbeat_data {
    pubnub_t* pb;
    pubnub_t* heartbeat_pb;
    size_t    period_sec;
};

struct HeartbeatWatcherData {
    struct pubnub_heartbeat_data heartbeat_data[PUBNUB_MAX_HEARTBEAT_THUMPERS] pubnub_guarded_by(mutw);
#if !defined(PUBNUB_CALLBACK_API) || defined(PUBNUB_NTF_RUNTIME_SELECTION)
    /** Array of thumper indices for which auto heartbeat transactions are currently in progress.
        Used in sync environment.
      */
    unsigned heartbeat_in_progress_index_array[PUBNUB_MAX_HEARTBEAT_THUMPERS] pubnub_guarded_by(mutw);
    /** Number of auto heartbeat transactions currently in progress. */
    unsigned heartbeats_in_progress pubnub_guarded_by(mutw);
#endif
    unsigned thumpers_in_use pubnub_guarded_by(mutw);
    /** Times left for each of the thumper timers in progress */
    size_t heartbeat_timers[PUBNUB_MAX_HEARTBEAT_THUMPERS] pubnub_guarded_by(timerlock);
    /** Array of thumper indexes whos auto heartbeat timers are active and running */
    unsigned timer_index_array[PUBNUB_MAX_HEARTBEAT_THUMPERS] pubnub_guarded_by(timerlock);
    /** Number of active thumper timers */
    unsigned active_timers pubnub_guarded_by(timerlock);
    bool stop_heartbeat_watcher_thread pubnub_guarded_by(stoplock);
    pubnub_mutex_t                     mutw;
    pubnub_mutex_t                     timerlock;
    pubnub_mutex_t                     stoplock;
    pubnub_thread_t                    thread_id;
    thread_handle_field()
};

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

/** Reads channel and channel groups saved(subscribed on)
  */
void pbauto_heartbeat_read_channelInfo(pubnub_t const* pb,
                                       char const** channel,
                                       char const** channel_group);

/** Manipulates thumpers in use */
pubnub_watcher_t pbauto_heartbeat_watcher_thread(void* arg);

/** Initializes and starts auto heartbeat watcher thread. Different on different platforms */
int pbauto_heartbeat_init(struct HeartbeatWatcherData* m_watcher);

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
#define pbauto_heartbeat_read_channelInfo(pb, channel, channel_group)
#define pbauto_heartbeat_watcher_thread(arg)
#define pbauto_heartbeat_init(m_watcher)
#define pbauto_heartbeat_transaction_ongoing(pb)
#define pbauto_heartbeat_start_timer(pb)
#define pbauto_heartbeat_free_channelInfo(pb)
#define pbauto_heartbeat_prepare_channels_and_ch_groups(pb, addr_channel, addr_channel_group) PNR_OK
#define pbauto_heartbeat_stop()
#endif /* PUBNUB_USE_AUTO_HEARTBEAT */

#endif /* !defined INC_PBAUTO_HEARTBEAT */

