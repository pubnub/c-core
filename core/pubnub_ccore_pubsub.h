/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_CCORE_PUBSUB
#define INC_PUBNUB_CCORE_PUBSUB

#if PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE
#include "pbcc_subscribe_event_engine.h"
#endif

#if PUBNUB_USE_RETRY_CONFIGURATION
#include "core/pubnub_retry_configuration.h"
#include "core/pbcc_request_retry_timer.h"
#endif // #if PUBNUB_USE_RETRY_CONFIGURATION

#include "pubnub_config.h"
#include "pubnub_api_types.h"
#include "pubnub_generate_uuid.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


/** @file pubnub_ccore_pubsub.h

    This has the functions for formating and parsing the requests and
    responses for publish and subscribe transactions.
*/

#if PUBNUB_CRYPTO_API

// assuming 50 bytes per message
#define PUBNUB_MAX_DECRYPTED_MESSAGES (PUBNUB_BUF_MAXLEN / 50) 

#endif // PUBNUB_CRYPTO_API

/** The Pubnub "(C) core" context, contains context data
    that is shared among all Pubnub C clients.
 */
struct pbcc_context {
    /** The publish key (to use when publishing) */
    char const* publish_key;
    /** The subscribe key (to use when subscribing) */
    char const* subscribe_key;
    /** The user_id to be sent to server. If empty string, don't send any */
    char *user_id;
    /** The user_id length */
    int user_id_len;
    /** The `auth` parameter to be sent to server. If NULL, don't send
     * any */
    char const* auth;
    /** The `auth_token` parameter to be sent to server. If NULL, don't send
     * any */
    char const* auth_token;
    /** Pointer to the message to send via POST method */
    char const* message_to_send;

    /** The last recived subscribe time token. */
    char timetoken[20];

    /** Pointer to the presence state */
    char const* state;

#if PUBNUB_USE_SUBSCRIBE_V2
    /** The last received subscribe V2 region */
    int region;
#endif

#if PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE
    // Event Engine which supports new subscription loop.
    pbcc_subscribe_ee_t *subscribe_ee;
#endif

    /** The result of the last Pubnub transaction */
    enum pubnub_res last_result;

    /** The "scratch" buffer for HTTP data */
    char http_buf[PUBNUB_BUF_MAXLEN];

    /** The length of the data currently in the HTTP buffer ("scratch"
        or reply, depending on the state).
     */
    size_t http_buf_len;

#if PUBNUB_CRYPTO_API
    /** Holds encrypted message */
    char encrypted_msg_buf[PUBNUB_BUF_MAXLEN];
#endif

#if PUBNUB_USE_GZIP_COMPRESSION
    /** Buffer for compressed message */
    char gzip_msg_buf[PUBNUB_COMPRESSED_MAXLEN];
    
    /** The length of compressed data in 'comp_http_buf' ready to be sent */
    size_t gzip_msg_len;
#endif

#if PUBNUB_RECEIVE_GZIP_RESPONSE
    /** The length of the decompressed data currently in the decompressing
     * buffer ("scratch").
     */
    size_t decomp_buf_size;
#endif
    /** The total length of data to be received in a HTTP reply or
        chunk of it.
     */
    unsigned http_content_len;

#if PUBNUB_DYNAMIC_REPLY_BUFFER
    char* http_reply;
#if PUBNUB_RECEIVE_GZIP_RESPONSE
    char* decomp_http_reply;
#endif /* PUBNUB_RECEIVE_GZIP_RESPONSE */
#else
    /** The contents of a HTTP reply/reponse */
    char http_reply[PUBNUB_REPLY_MAXLEN + 1];
#if PUBNUB_RECEIVE_GZIP_RESPONSE
    /** Auxiliary buffer for unpacking(decompresing) data from HTTP reply buffer
     */
    char decomp_http_reply[PUBNUB_REPLY_MAXLEN + 1];
#endif /* PUBNUB_RECEIVE_GZIP_RESPONSE */
#endif /* PUBNUB_DYNAMIC_REPLY_BUFFER */

    /* These in-string offsets are used for yielding messages received
     * by subscribe - the beginning of last yielded message and total
     * length of message buffer.
     */
    unsigned msg_ofs, msg_end;

    /* Like the offsets for the messages, these are the offsets for
       the channels. Unlikey the message(s), the channels don't have
       to be received at all.
    */
    unsigned chan_ofs, chan_end;

#if PUBNUB_USE_RETRY_CONFIGURATION
    /** Pointer to the configuration with failed request handling details. */
    pubnub_retry_configuration_t* retry_configuration;
    /** Failed request retry timer. */
    pbcc_request_retry_timer_t* retry_timer;
    /** Current number of retry attempts. */
    int http_retry_count;
#endif // #if PUBNUB_USE_RETRY_CONFIGURATION

#if PUBNUB_CRYPTO_API
    /** Secret key to use for encryption/decryption */
    char const* secret_key;

    /** Crypto module for encryption and decryption */
    struct pubnub_crypto_provider_t *crypto_module;

    /** Holds decrypted messages */
    uint8_t* decrypted_messages[PUBNUB_MAX_DECRYPTED_MESSAGES]; 

    /** The length of decrypted messages */
    size_t decrypted_message_count;

#endif // PUBNUB_CRYPTO_API
};


#define APPEND_URL_PARAM_M(pbc, name, var, separator)                          \
    if ((var) != NULL) {                                                       \
        const char* param_ = name;            \
        enum pubnub_res rslt_    = pbcc_append_url_param(                      \
            (pbc), param_, sizeof(param_)-1 , (var), (separator));             \
        if (rslt_ != PNR_OK) {                                                 \
            return rslt_;                                                      \
        }                                                                      \
    }

#define APPEND_URL_LITERAL_M_IMP(pbc, string_literal)                          \
    do {                                                                       \
        if ((pbc)->http_buf_len + sizeof(string_literal) > sizeof (pbc)->http_buf) {\
            PUBNUB_LOG_ERROR("Error: Request buffer too small - cannot append url literal:\n"\
                             "current_buffer_size = %lu\n"                     \
                             "required_buffer_size = %lu\n",                   \
                             (unsigned long)(sizeof (pbc)->http_buf),          \
                             (unsigned long)((pbc)->http_buf_len + 1 + sizeof(string_literal)));\
            return PNR_TX_BUFF_TOO_SMALL;                                      \
        }                                                                      \
        strcpy((pbc)->http_buf + (pbc)->http_buf_len, (string_literal));       \
        (pbc)->http_buf_len += sizeof(string_literal) - 1;                     \
    } while (0)

#define APPEND_URL_LITERAL_M(pbc, string_literal) APPEND_URL_LITERAL_M_IMP(pbc, "" string_literal)

#define APPEND_URL_STRING_M(pbc, s, n)                                         \
    do {                                                                       \
        char const* M_s_ = s;                                                  \
        if (M_s_ != NULL) {                                                    \
            struct pbcc_context* M_pbc_ = pbc;                                 \
            size_t M_n_ = n;                                                   \
            if (M_pbc_->http_buf_len + M_n_ > sizeof M_pbc_->http_buf) {       \
                PUBNUB_LOG_ERROR("Error: Request buffer too small - cannot append url string:\n"\
                                 "current_buffer_size = %lu\n"                 \
                                 "required_buffer_size = %lu\n",               \
                                 (unsigned long)(sizeof M_pbc_->http_buf),     \
                                 (unsigned long)(M_pbc_->http_buf_len + 1 + M_n_));\
                return PNR_TX_BUFF_TOO_SMALL;                                  \
            }                                                                  \
            M_pbc_->http_buf_len += snprintf(M_pbc_->http_buf + M_pbc_->http_buf_len,\
                                             sizeof M_pbc_->http_buf - M_pbc_->http_buf_len - 1,\
                                             "%.*s",                           \
                                             (int)M_n_,                        \
                                             M_s_);                            \
        }                                                                      \
    } while (0)

#define APPEND_URL_ENCODED_M(pbc, what)                                        \
    if ((what) != NULL) {                                                      \
        enum pubnub_res rslt_ = pbcc_url_encode((pbc), (what), PBTT_NONE);     \
        if (rslt_ != PNR_OK) {                                                 \
            return rslt_;                                                      \
        }                                                                      \
    }

#define APPEND_URL_ENCODED_M_TRANS(tr, pbc, what)                                        \
    if ((what) != NULL) {                                                      \
        enum pubnub_res rslt_ = pbcc_url_encode((pbc), (what), tr);     \
        if (rslt_ != PNR_OK) {                                                 \
            return rslt_;                                                      \
        }                                                                      \
    }

#define APPEND_URL_PARAM_ENCODED_M(pbc, name, var, separator)                  \
    if ((var) != NULL) {                                                       \
        const char* param_ = name;                                             \
        enum pubnub_res rslt_    = pbcc_append_url_param_encoded(              \
            (pbc), param_, sizeof(param_)-1, (var), (separator), PBTT_NONE);   \
        if (rslt_ != PNR_OK) {                                                 \
            return rslt_;                                                      \
        }                                                                      \
    }

#define APPEND_URL_PARAM_INT_M(pbc, name, var, separator)                      \
    do {                                                                       \
        char v_[20];                                                           \
        snprintf(v_, sizeof v_, "%d", (var));                                  \
        APPEND_URL_PARAM_M(pbc, name, v_, separator);                          \
    } while (0)


#define APPEND_URL_PARAM_UNSIGNED_M(pbc, name, var, separator)                 \
    do {                                                                       \
        char v_[20];                                                           \
        snprintf(v_, sizeof v_, "%u", (var));                                  \
        APPEND_URL_PARAM_M(pbc, name, v_, separator);                          \
    } while (0)

#define APPEND_URL_OPT_PARAM_UNSIGNED_M(pbc, name, var, separator)             \
    if ((var) != NULL) {                                                       \
        char v_[20];                                                           \
        snprintf(v_, sizeof v_, "%u", *(var));                                 \
        APPEND_URL_PARAM_M(pbc, name, v_, separator);                          \
    }                                                                          \
    while (0)


#define APPEND_URL_PARAM_BOOL_M(pbc, name, var, separator)                     \
    do {                                                                       \
        char const* v_ = (var) ? "true" : "false";                             \
        APPEND_URL_PARAM_M(pbc, name, v_, separator);                          \
    } while (0)

#define APPEND_URL_PARAM_TRIBOOL_M(pbc, name, var, separator)                  \
    if ((var) != pbccNotSet) {                                                 \
        char const* v_ = (pbccTrue == var) ? "1" : "0";                        \
        APPEND_URL_PARAM_M(pbc, name, v_, separator);                          \
    }

#define APPEND_URL_PARAM_TRIBOOL_SIMBOL_M(pbc, name, var, separator)           \
    if ((var) != pbccNotSet) {                                                 \
        char const* v_ = (pbccTrue == var) ? "true" : "false";                 \
        APPEND_URL_PARAM_M(pbc, name, v_, separator);                          \
    }

#if PUBNUB_USE_GZIP_COMPRESSION
#define NOT_COMPRESSED_AND(pbc) ((pbc)->gzip_msg_len == 0) &&
#define CHECK_IF_GZIP_COMPRESSED(pbc, message)                                 \
    if ((pbc)->gzip_msg_len != 0) {                                            \
        (pbc)->message_to_send = (message);                                    \
    }                                                                          \
    else {                                                                     \
        (pbc)->message_to_send = strcpy((pbc)->http_buf + (pbc)->http_buf_len + 1,\
                                        (message));                            \
    }
#else
#define NOT_COMPRESSED_AND(pbc)
#define CHECK_IF_GZIP_COMPRESSED(pbc, message)                                 \
    (pbc)->message_to_send = strcpy((pbc)->http_buf + (pbc)->http_buf_len + 1, \
                                    (message))
#endif /* PUBNUB_USE_GZIP_COMPRESSION */

#define APPEND_MESSAGE_BODY_M(rslt, pbc, message)                              \
    if ((PNR_OK == (rslt)) && ((message) != NULL)) {                           \
        if (NOT_COMPRESSED_AND(pbc)(pb_strnlen_s(message, PUBNUB_MAX_OBJECT_LENGTH) >\
                                    sizeof (pbc)->http_buf - (pbc)->http_buf_len - 2)) {\
            PUBNUB_LOG_ERROR("Error: Request buffer too small - cannot pack the message body:\n"\
                             "current_buffer_size = %lu\n"                     \
                             "required_buffer_size = %lu\n",                   \
                             (unsigned long)(sizeof (pbc)->http_buf),          \
                             (unsigned long)((pbc)->http_buf_len + 2 + pb_strnlen_s(message,\
                                                                                    PUBNUB_MAX_OBJECT_LENGTH)));\
            return PNR_TX_BUFF_TOO_SMALL;                                      \
        }                                                                      \
        (pbc)->http_buf[(pbc)->http_buf_len] = '\0';                           \
        CHECK_IF_GZIP_COMPRESSED((pbc), (message));                            \
    }

#define URL_PARAMS_INIT(name, size)                                            \
	struct  name##_data {                                                      \
        const char* param_key;                                                 \
        size_t key_size;                                                       \
        const char* param_val;                                                 \
    };                                                                         \
    struct name##_data name[(size)];                                           \
    memset(&name, 0, sizeof(struct name##_data) * (size));                     \
    char name##_index = 0;                                                     \
    int name##_count = 0;                                                      \
    char name##_val_str[21];                                                   \
    (void)name##_val_str;

#define ADD_URL_PARAM(name, key, value)                                     \
    name[(int)(name##_index)].param_key = #key;                             \
    name[(int)(name##_index)].key_size = sizeof(#key)-1;                    \
    name[(int)(name##_index)].param_val = value;                            \
    (int)(name##_index)++;

#define ADD_URL_AUTH_PARAM(pb, name, key)                                     \
        if (pb->auth_token != NULL) { ADD_URL_PARAM(name, key, pb->auth_token); } \
        else if (pb->auth != NULL) { ADD_URL_PARAM(name, key, pb->auth); }        

#define ADD_URL_PARAM_SIZET(name, key, value)                               \
    int val_len = snprintf(NULL, 0, "%lu", (long unsigned int)value);       \
    if (val_len >= 21) {                                                    \
        PUBNUB_LOG_ERROR("Error: in ADD_URL_PARAM_SIZET:\n");               \
        return PNR_TX_BUFF_TOO_SMALL;                                       \
    }                                                                       \
    snprintf(name##_val_str, val_len + 1, "%lu", (long unsigned int)value); \
    name[(int)(name##_index)].param_key = #key;                             \
    name[(int)(name##_index)].key_size = sizeof(#key)-1;                    \
    name[(int)(name##_index)].param_val = name##_val_str;                   \
    (int)(name##_index)++;

#define ADD_URL_PARAM_TRUE_KEY(name, key, value)                          \
    name[(int)(name##_index)].param_key = key;                            \
    name[(int)(name##_index)].key_size = strlen(key);                     \
    name[(int)(name##_index)].param_val = value;                          \
    (int)(name##_index)++;

#define ADD_TS_TO_URL_PARAM()                                               \
    time_t epoch_time = time(NULL);                                         \
    char timestamp[16];                                                     \
    if (epoch_time > 0) {                                                   \
        sprintf(timestamp, "%lld", (long long)epoch_time);                  \
        ADD_URL_PARAM(qparam, timestamp, timestamp);                        \
    }

#define SORT_URL_PARAMETERS(name)                                              \
    struct name##_data temp;                                                   \
    name##_count = sizeof(name) / sizeof(*name);                               \
    for (int i = 0; i < name##_count - 1; i++) {                               \
        for (int j = 0; j < name##_count; j++) {                               \
            if (name[i].param_key != NULL &&                                   \
                name[j].param_key != NULL &&                                   \
                (strcmp(name[i].param_key, name[j].param_key) < 0)) {          \
                    temp = name[i];                                            \
                    name[i] = name[j];                                         \
                    name[j] = temp;                                            \
            }                                                                  \
        }                                                                      \
    }

#define ENCODE_URL_PARAMETERS(pb, name)                                        \
    name##_count = sizeof(name) / sizeof(*name);                               \
    for (int i = 0; i < name##_count - 1; i++) {                               \
        if (name[i].param_key != NULL && name[i].param_val != NULL) {          \
            enum pubnub_res rslt_ = pbcc_append_url_param_encoded(             \
                (pb), (name[i].param_key), name[i].key_size,                   \
                (name[i].param_val), (i == 0 ? '?' : '&'), PBTT_NONE);         \
            if (rslt_ != PNR_OK)                                               \
                return rslt_;                                                  \
        }                                                                      \
    }

#define ENCODE_URL_PARAMS_TRANS(tr, pb, name)                                  \
    name##_count = sizeof(name) / sizeof(*name);                               \
    for (int i = 0; i < name##_count - 1; i++) {                               \
        if (name[i].param_key != NULL && name[i].param_val != NULL) {          \
            enum pubnub_res rslt_ = pbcc_append_url_param_encoded(             \
                (pb), (name[i].param_key), name[i].key_size,                   \
                (name[i].param_val), (i == 0 ? '?' : '&'), tr);                \
            if (rslt_ != PNR_OK)                                               \
                return rslt_;                                                  \
        }                                                                      \
    }

/** Initializes the Pubnub C core context */
void pbcc_init(struct pbcc_context* pbcc,
               const char*          publish_key,
               const char*          subscribe_key);

/** Deinitializes the Pubnub C core context */
void pbcc_deinit(struct pbcc_context* p);

/** Reallocates the reply buffer in the C core context @p p to have
    @p bytes.
    @return 0: OK, allocated, -1: failed
*/
int pbcc_realloc_reply_buffer(struct pbcc_context* p, unsigned bytes);

/** Ensures existence of reply buffer in the C core context @p p
    in special cases when no: 'Content-Length:', nor 'Transfer-Encoding:
   chunked' header line has been received.
    @return true: OK(at least one byte allocated), false: otherwise
*/
bool pbcc_ensure_reply_buffer(struct pbcc_context* p);

/** Returns the next message from the Pubnub C Core context. NULL if
    there are no (more) messages
*/
char const* pbcc_get_msg(struct pbcc_context* pb);

/** Returns the next channel from the Pubnub C Core context. NULL if
    there are no (more) messages.
*/
char const* pbcc_get_channel(struct pbcc_context* pb);

/** Sets the user_id for the context */
enum pubnub_res pbcc_set_user_id(struct pbcc_context* pb, const char* user_id);

/** Returns the user_id for the context */
char const* pbcc_user_id_get(struct pbcc_context* pb);

/** Sets the `auth` for the context */
void pbcc_set_auth(struct pbcc_context* pb, const char* auth);

/** Sets the `auth` for the context */
void pbcc_set_auth_token(struct pbcc_context* pb, const char* token);

/** Response parser function prototype */
typedef enum pubnub_res (*PFpbcc_parse_response_T)(struct pbcc_context*);

/** Parses the string received as a response for a subscribe operation
    (transaction). This checks if the response is valid, and, if it
    is, prepares for giving the messages (and possibly channels) that
    are received in the response to the user (via pbcc_get_msg() and
    pbcc_get_channel()).

    @param p The Pubnub C core context to parse the response "in"
    @return PNR_OK: OK, PNR_FORMAT_ERROR: error (invalid response)
*/
enum pubnub_res pbcc_parse_subscribe_response(struct pbcc_context* p);

/** Parses the string received as a response for a publish operation
    (transaction). This checks if the response is valid, and, if it
    is, enables getting it as the gotten message (like for
    `subscribe`).

    @param p The Pubnub C core context to parse the response "in"
    @return The result of the parsing, expressed as the "Pubnub
    result" enum
*/
enum pubnub_res pbcc_parse_publish_response(struct pbcc_context* p);

/** Prepares HTTP header lines specific for sending message 'via POST' method.
    @param p The Pubnub C core context with all necessary information
    @param header pointer to char array provided for placing these headers
    @param max_length maximum size of array provided
 */
void pbcc_via_post_headers(struct pbcc_context* p, char* header, size_t max_length);

/** Prepares the Publish operation (transaction), mostly by
    formatting the URI of the HTTP request.
 */
enum pubnub_res pbcc_publish_prep(struct pbcc_context* pb,
                                  const char*          channel,
                                  const char*          message,
                                  bool                 store_in_history,
                                  bool                 norep,
                                  char const*          meta,
                                  size_t               ttl,
                                  enum pubnub_method   method);

/** Prepares the Signal operation (transaction), mostly by
    formatting the URI of the HTTP request.
 */
enum pubnub_res pbcc_signal_prep(struct pbcc_context* pb,
                                 const char* channel,
                                 const char* message);

/** Prepares the Subscribe operation (transaction), mostly by
    formatting the URI of the HTTP request.
 */
enum pubnub_res pbcc_subscribe_prep(struct pbcc_context* p,
                                    char const*          channel,
                                    char const*          channel_group,
                                    unsigned*            heartbeat);


/** Split @p buf string containing a JSON array (with arbitrary
 * contents) to multiple NUL-terminated C strings, in-place.
 */
bool pbcc_split_array(char* buf);


enum pubnub_res pbcc_append_url_param(struct pbcc_context* pb,
                                      char const*          param_name,
                                      size_t               param_name_len,
                                      char const*          param_val,
                                      char                 separator);

enum pubnub_res pbcc_url_encode(struct pbcc_context* pb, char const* what, enum pubnub_trans pt);

enum pubnub_res pbcc_append_url_param_encoded(struct pbcc_context* pb,
                                              char const*          param_name,
                                              size_t      param_name_len,
                                              char const* param_val,
                                              char        separator, 
                                              enum pubnub_trans pt);

enum pubnub_res pbcc_sign_url(struct pbcc_context* pc,
                                const char* msg,
                                enum pubnub_method method,
                                bool v3sign);

#endif /* !defined INC_PUBNUB_CCORE_PUBSUB */
