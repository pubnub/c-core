/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if PUBNUB_PROXY_API

#if !defined INC_PBNTLM_PACKER
#define      INC_PBNTLM_PACKER

#include "pubnub_internal.h"

#include "pubnub_memory_block.h"


void pbntlm_packer_init(pbntlm_ctx_t *pb);

int pbntlm_pack_type_one(pbntlm_ctx_t *pb, char const* username, char const* password, pubnub_bymebl_t* msg);

int pbntlm_unpack_type2(pbntlm_ctx_t *pb, pubnub_bymebl_t msg);

int pbntlm_pack_type3(pbntlm_ctx_t *pb, char const* username, char const* password, pubnub_bymebl_t* msg);

void pbntlm_packer_deinit(pbntlm_ctx_t *pb);


#endif /* !defined INC_PBNTLM_PACKER */
#endif /* PUBNUB_PROXY_API */

