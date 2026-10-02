/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_GRANT_TOKEN_API
#define INC_PUBNUB_GRANT_TOKEN_API


#include "pubnub_api_types.h"
#include "pubnub_memory_block.h"

#include <stdbool.h>
#include "lib/cbor/cbor.h"
#include "lib/pb_extern.h"

struct pam_permission{
    bool read;
    bool write;
    bool manage;
    bool del;
    bool create;
    bool get;
    bool update;
    bool join;
};

PUBNUB_EXTERN int pubnub_get_grant_bit_mask_value(struct pam_permission pam);


/** Returns the token for a set of permissions specified in @p perm_obj.
    An example for @perm_obj:
    {
      "ttl":1440,
      "permissions":{
          "resources":{
            "channels":{ "mych":31 },
            "groups":{ "mycg":31 },
            "uuids":{ "myuuid":31 },
            "users":{ "myuser":31 },
            "spaces":{ "myspc":31 }
          },
          "patterns":{
            "channels":{ },
            "groups":{ },
            "uuids":{ "^$":1 },
            "users":{ "^$":1 },
            "spaces":{ "^$":1 }
          },
          "categories":{
            "channels":32,
            "uuids":32
          },
          "meta":{ }
      }
    }

    `categories` grants App Context enumeration for the whole keyset.
    Only the GET bit (32) is valid, and only on `channels` and `uuids`.
    A body that has `categories` and no resources or patterns is valid.
    The string is sent as the POST body without filtering.

    @param pb The pubnub context. Can't be NULL
    @param perm_obj The JSON string with resource, pattern, and category permissions.
    @return #PNR_STARTED on success, an error otherwise
  */
PUBNUB_EXTERN enum pubnub_res pubnub_grant_token(pubnub_t* pb, char const* perm_obj);

PUBNUB_EXTERN pubnub_chamebl_t pubnub_get_grant_token(pubnub_t* pb);

/** Parses the @p token and returns the json string.
   
    @see pubnub_grant_token
    @return malloc allocated char pointer (must be passed to `free` to avoid a memory leak)
*/
PUBNUB_EXTERN char* pubnub_parse_token(pubnub_t* pb, char const* token);

#endif /* !defined INC_PUBNUB_GRANT_TOKEN_API */
