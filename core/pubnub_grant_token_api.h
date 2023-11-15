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
          "meta":{ }
      }
    }

    @param pb The pubnub context. Can't be NULL
    @param perm_obj The JSON string with the permissions for resources and patterns.
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
