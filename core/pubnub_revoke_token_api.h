/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */


/** Revokes the token specified in @p token.

    @param pb The pubnub context. Can't be NULL
    @param token The Token string.
    @return #PNR_STARTED on success, an error otherwise
  */

enum pubnub_res pubnub_revoke_token(pubnub_t* pb, char const* token);


/** Returns the result of the revoke token operation.

    @param pb The pubnub context. Can't be NULL
    @return pubnub_chamebl_t with pointer to string with response
  */
pubnub_chamebl_t pubnub_get_revoke_token_response(pubnub_t* pb);
