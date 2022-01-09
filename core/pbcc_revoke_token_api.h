/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PBCC_REVOKE_TOKEN_API
#define INC_PBCC_REVOKE_TOKEN_API

#include "pubnub_api_types.h"
#include "pubnub_json_parse.h"
#include "pubnub_memory_block.h"

struct pbcc_context;

enum pubnub_res pbcc_revoke_token_prep(struct pbcc_context* pb, char const* token, enum pubnub_trans pt);

pubnub_chamebl_t pbcc_get_revoke_token_response(struct pbcc_context* pb);

enum pubnub_res pbcc_parse_revoke_token_response(struct pbcc_context* pb);

#endif /* !defined INC_PBCC_REVOKE_TOKEN_API */
