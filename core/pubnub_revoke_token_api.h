/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#ifndef INC_PUBNUB_REVOKE_TOKEN_API
#define	INC_PUBNUB_REVOKE_TOKEN_API

#include "pubnub_api_types.h"
#include "pubnub_memory_block.h"
#include "lib/pb_extern.h"

/** Revokes the token specified in @p token.

    @param pb The pubnub context. Can't be NULL
    @param token The Token string.
    @return #PNR_STARTED on success, an error otherwise
  */

PUBNUB_EXTERN enum pubnub_res pubnub_revoke_token(pubnub_t* pb, char const* token);


/** Returns the result of the revoke token operation.

    @param pb The pubnub context. Can't be NULL
    @return pubnub_chamebl_t with pointer to string with response
  */
PUBNUB_EXTERN pubnub_chamebl_t pubnub_get_revoke_token_response(pubnub_t* pb);

#endif /* INC_PUBNUB_REVOKE_TOKEN_API */
