/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbpal_add_system_certs.h"

#include "pubnub_internal.h"
#include "core/pubnub_log.h"

#include "openssl/x509.h"

#include <windows.h>

#include <wincrypt.h>

#pragma comment(lib, "crypt32")

int pbpal_add_system_certs(pubnub_t* pb)
{
    X509_STORE *cert_store = SSL_CTX_get_cert_store(pb->pal.ctx);
    #if __UWP__
    HCERTSTORE hStore = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, NULL, CERT_SYSTEM_STORE_CURRENT_USER, L"ROOT");
    #else
    HCERTSTORE hStore = CertOpenSystemStoreW(0, L"ROOT");
    #endif
    PCCERT_CONTEXT pContext = NULL;

    if (!hStore) {
        return -1;
    }

    while (pContext = CertEnumCertificatesInStore(hStore, pContext)) {
        X509* x509 = d2i_X509(NULL, (const unsigned char **)&pContext->pbCertEncoded, pContext->cbCertEncoded);
        if (x509 != NULL) {
            if (0 == X509_STORE_add_cert(cert_store, x509)) {
                PUBNUB_LOG_ERROR("X509_STORE_add_cert() failed for Windows system certificate");
            }

            X509_free(x509);
        }
    }

    CertCloseStore(hStore, 0);

    return 0;
}
