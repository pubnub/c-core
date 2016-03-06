/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_HELPER
#define	INC_PUBNUB_HELPER


#include "pubnub_api_types.h"


/** @file pubnub_helper.h 

    This is a set of "Helper" functions of the Pubnub client library.
    You don't need this to work with Pubnub client, but they may come
    in handy.
*/


/** Possible (known) publish results (outcomes) in the description
    received from Pubnub.
 */
enum pubnub_publish_res {
  /** Publish succeeded, message sent on the channel */
  PNPUB_SENT,
  /** Publish failed, the message had invalid JSON */
  PNPUB_INVALID_JSON,
  /** Publish failed, the channel had an invalid character in the name */
  PNPUB_INVALID_CHAR_IN_CHAN_NAME,
  /** The publishing quota for this account was exceeded */
  PNPUB_ACCOUNT_QUOTA_EXCEEDED,
  /** Message is too large to be published */
  PNPUB_MESSAGE_TOO_LARGE,
  /** Publish failed, but we were not able to parse the error description */
  PNPUB_UNKNOWN_ERROR,
};

/** Parses the given publish @p result. You usually obtain this with
    pubnub_last_publish_result().
 */
enum pubnub_publish_res pubnub_parse_publish_result(char const *result);

/** Returns a string (in English) describing a Pubnub result enum value 
    @p e.
 */
char const* pubnub_res_2_string(enum pubnub_res e);


#endif /* defined INC_PUBNUB_HELPER */
