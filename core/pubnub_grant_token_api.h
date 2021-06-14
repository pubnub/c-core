/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_GRANT_TOKEN_API
#define INC_PUBNUB_GRANT_TOKEN_API


#include "pubnub_api_types.h"

#include <stdbool.h>

int pubnub_get_grant_bit_mask_value(bool read, 
                                  bool write, 
                                  bool manage, 
                                  bool del, 
                                  bool create);

/** Returns the token for a set of permissions specified in @p perm_obj.
    An example for @perm_obj:
    {
      "ttl":1440,
      "permissions":{
          "resources":{
            "channels":{ "mych":31 },
            "groups":{ "mycg":31 },
            "users":{ "myuser":31 },
            "spaces":{ "myspc":31 }
          },
          "patterns":{
            "channels":{ },
            "groups":{ },
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

#endif /* !defined INC_PUBNUB_GRANT_TOKEN_API */
