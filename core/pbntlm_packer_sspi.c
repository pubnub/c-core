/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if PUBNUB_PROXY_API

#include "pbntlm_packer.h"

#include "pubnub_log.h"
#include "pubnub_assert.h"


#pragma comment(lib, "secur32")


ULONG static get_max_token(void)
{
    ULONG result;
    PSecPkgInfo pkg;
    SECURITY_STATUS status = QuerySecurityPackageInfoA((char*)"NTLM", &pkg);
    if (status != SEC_E_OK) {
        return 0;
    }
    result = pkg->cbMaxToken;
    FreeContextBuffer(pkg);

    return result;
}


static void fill_sspi_identity(SEC_WINNT_AUTH_IDENTITY *identity, char const* username, char const* password)
{
    PUBNUB_ASSERT_OPT(NULL != identity);
    PUBNUB_ASSERT_OPT(NULL != username);
    PUBNUB_ASSERT_OPT(NULL != password);

    identity->User = (unsigned char*)username;
    identity->UserLength = strlen(username);
    
    /* For now, don't use the domain */
    identity->Domain = (unsigned char*)"";
    identity->DomainLength = 0;
    
    identity->Password = (unsigned char*)password;
    identity->PasswordLength = strlen(password);
    
    identity->Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
}


void pbntlm_packer_init(pbntlm_ctx_t *pb)
{
    ULONG sspi_max_token = get_max_token();

    PUBNUB_ASSERT_OPT(pb != NULL);

    if (sspi_max_token <= PUBNUB_NTLM_MAX_TOKEN) {
        PUBNUB_LOG_WARNING("PUBNUB_NTLM_MAX_TOKEN(==%d) is smaller than maximum NTLM token for SSPI, which is: %lu\n", PUBNUB_NTLM_MAX_TOKEN, sspi_max_token);
    }

    SecInvalidateHandle(&pb->hcontext);
    SecInvalidateHandle(&pb->hcreds);
    pb->in_token_size = 0;
}


static void fill_sec_buf_dec_out(SecBuffer *sec_buf, SecBufferDesc *sec_desc, pubnub_bymebl_t const* msg)
{
    sec_desc->ulVersion = SECBUFFER_VERSION;
    sec_desc->cBuffers = 1;
    sec_desc->pBuffers = sec_buf;
    sec_buf->BufferType = SECBUFFER_TOKEN;
    sec_buf->pvBuffer = msg->ptr;
    sec_buf->cbBuffer = msg->size;
}


static SECURITY_STATUS acquire_creds(PSEC_WINNT_AUTH_IDENTITY pident, CredHandle *phcreds)
{
    TimeStamp expiry;
    PUBNUB_ASSERT(!SecIsValidHandle(phcreds));
    return AcquireCredentialsHandleA(NULL,
                                     (char*)"NTLM",
                                     SECPKG_CRED_OUTBOUND, 
                                     NULL,
                                     pident, 
                                     NULL, 
                                     NULL,
                                     phcreds, 
                                     &expiry
        );
}


static SECURITY_STATUS init_sec_ctx(CredHandle *phcreds, CtxtHandle *phctx, SecBufferDesc *in_desc, CtxtHandle *phctx_new, SecBufferDesc *out_desc)
{
    ULONG context_attrs;
    PUBNUB_ASSERT((NULL == phctx) || SecIsValidHandle(phctx));
    PUBNUB_ASSERT_OPT(NULL != phctx_new);
    return InitializeSecurityContextA(phcreds, 
                                      phctx,
                                      (char*)"",
                                      0, 
                                      0, 
                                      SECURITY_NETWORK_DREP,
                                      in_desc, 
                                      0,
                                      phctx_new, 
                                      out_desc,
                                      &context_attrs, 
                                      NULL
        );
}


int pbntlm_pack_type_one(pbntlm_ctx_t *pb, char const* username, char const* password, pubnub_bymebl_t* msg)
{
    SecBuffer sec_buf;
    SecBufferDesc sec_desc;
    SECURITY_STATUS status;
    SEC_WINNT_AUTH_IDENTITY *pident;

    if ((NULL != username) && (username[0] != '\0')) {
        fill_sspi_identity(&pb->identity, username, password);
        pident = &pb->identity;
    }
    else {
        pident = NULL;
    }
    status = acquire_creds(pident, &pb->hcreds);
    if (status != SEC_E_OK) {
        PUBNUB_LOG_ERROR("Failed to acquire credentials for NTLM, code: %ld\n", status);
        return -1;
    }

    fill_sec_buf_dec_out(&sec_buf, &sec_desc, msg);

    status = init_sec_ctx(&pb->hcreds, NULL, NULL, &pb->hcontext, &sec_desc);
    switch (status) {
    case SEC_I_COMPLETE_NEEDED:
    case SEC_I_COMPLETE_AND_CONTINUE:
        CompleteAuthToken(&pb->hcontext, &sec_desc);
        /*FALLTHRU*/
    case SEC_E_OK:
    case SEC_I_CONTINUE_NEEDED:
        msg->size = sec_buf.cbBuffer;
        return 0;
    default:
        PUBNUB_LOG_ERROR("Failed to init context for NTLM type 1 message, code: %ld\n", status);
        return -1;
    }
}


int pbntlm_unpack_type2(pbntlm_ctx_t *pb, pubnub_bymebl_t msg)
{
    PUBNUB_ASSERT_OPT(pb != NULL);
    PUBNUB_ASSERT_OPT(msg.ptr != NULL);
    PUBNUB_ASSERT_OPT(msg.size > 0);

    if (msg.size > sizeof pb->in_token) {
        PUBNUB_LOG_ERROR("NTLM Type2 message too long: %zu bytes, max allowed %zu\n", msg.size, sizeof pb->in_token);
        return -1;
    }
    memcpy(pb->in_token, msg.ptr, msg.size);
    pb->in_token_size = msg.size;

    return 0;
}


int pbntlm_pack_type3(pbntlm_ctx_t *pb, char const* username, char const* password, pubnub_bymebl_t* msg)
{
    SecBuffer type_two_buf;
    SecBufferDesc type_two_desc;
    SecBuffer type_3_buf;
    SecBufferDesc type_3_desc;
    SECURITY_STATUS status;

    /* For SSPI, we don't need the username and password at this point */
    PUBNUB_UNUSED(username);
    PUBNUB_UNUSED(password);

    type_two_desc.ulVersion = SECBUFFER_VERSION;
    type_two_desc.cBuffers = 1;
    type_two_desc.pBuffers = &type_two_buf;
    type_two_buf.BufferType = SECBUFFER_TOKEN;
    type_two_buf.pvBuffer = pb->in_token;
    PUBNUB_ASSERT_OPT(pb->in_token_size > 0);
    type_two_buf.cbBuffer = pb->in_token_size;

    fill_sec_buf_dec_out(&type_3_buf, &type_3_desc, msg);

    PUBNUB_ASSERT(SecIsValidHandle(&pb->hcreds));
    status = init_sec_ctx(&pb->hcreds, &pb->hcontext, &type_two_desc, &pb->hcontext, &type_3_desc);
    if (status != SEC_E_OK) {
        PUBNUB_LOG_ERROR("NTLM handshake failure (type-3 message): Status=%lx\n", status);
        
        return -1;
    }

    msg->size = type_3_buf.cbBuffer;

    return 0;
}


void pbntlm_packer_deinit(pbntlm_ctx_t *pb)
{
    if (SecIsValidHandle(&pb->hcontext)) {
        DeleteSecurityContext(&pb->hcontext);
        SecInvalidateHandle(&pb->hcontext);
    }

    if (SecIsValidHandle(&pb->hcreds)) {
        FreeCredentialsHandle(&pb->hcreds);
        SecInvalidateHandle(&pb->hcreds);
    }
}

#endif /* PUBNUB_PROXY_API */

