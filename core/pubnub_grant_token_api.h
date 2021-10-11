/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_GRANT_TOKEN_API
#define INC_PUBNUB_GRANT_TOKEN_API


#include "pubnub_api_types.h"

#include <stdbool.h>
#include "lib/cbor/cbor.h"

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

int pubnub_get_grant_bit_mask_value(struct pam_permission pam);


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
enum pubnub_res pubnub_grant_token(pubnub_t* pb, char const* perm_obj);

pubnub_chamebl_t pubnub_get_grant_token(pubnub_t* pb);

char* pubnub_parse_token(pubnub_t* pb, char const* token);

static CborError data_recursion(CborValue* it, int nestingLevel, char* json_result);

/** Set the auth token information of PubNub client context @p
    p. Pass NULL to unset.

    @note The @p token is expected to be valid (ASCIIZ string) pointers
    throughout the use of context @p pb, that is, until either you call
    pubnub_done() on @p pb, or the otherwise stop using it (like when
    the whole software/ firmware stops working). So, the contents of
    the auth string is not copied to the Pubnub context @p pb.  */
void pubnub_set_auth_token(pubnub_t* pb, const char* token);

/** Returns the current auth token information for the
    context @p pb.
    After pubnub_init(), it will return `NULL` until you change it
    to non-`NULL` via pubnub_set_auth_token().
*/
char const* pubnub_auth_token_get(pubnub_t* pb);

#endif /* !defined INC_PUBNUB_GRANT_TOKEN_API */
