/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PBCC_GRANT_TOKEN_API
#define INC_PBCC_GRANT_TOKEN_API

#include "pubnub_api_types.h"
#include "pubnub_json_parse.h"
#include "pubnub_memory_block.h"

struct pbcc_context;


/** @file pbcc_grant_token_api.h

    This has the functions for formating and parsing the
    requests and responses for 'Grant Token API' transactions
  */

/** Prepares the 'grant_token_prep' transaction, mostly by
    formatting the URI of the HTTP request.
  */
enum pubnub_res pbcc_grant_token_prep(struct pbcc_context* pb,
                                      const char* perm_obj,
                                      enum pubnub_trans pt);

/** Parses server response simply on any 'Grant Token API' transaction request.
    If transaction is successful, the response(a JSON object) will have key
    "data" with corresponding value. If not, there should be "error" key 'holding'
    error description. If there is neither of the two keys mentioned function
    returns response format error.
    Complete answer will be available via pubnub_get().

    @retval PNR_OK on success
    @retval PNR_GRANT_TOKEN_API_ERROR on error
    @retval PNR_FORMAT_ERROR no "data", nor "error" key present in response
  */
enum pubnub_res pbcc_parse_grant_token_api_response(struct pbcc_context* pb);

pubnub_chamebl_t pbcc_get_grant_token(struct pbcc_context* pb);

#endif /* !defined INC_PBCC_GRANT_TOKEN_API */
