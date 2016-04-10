/*******************************************************************************
 Source file for the CyaSSL glue functions to work with Harmony


  Summary:


  Description:

*******************************************************************************/

/*******************************************************************************
File Name: net_tls_cyassl_glue.c
Copyright ? 2013 released Microchip Technology Inc.  All rights
reserved.

Microchip licenses to you the right to use, modify, copy and distribute
Software only when embedded on a Microchip microcontroller or digital signal
controller that is integrated into your product or third party product
(pursuant to the sublicense terms in the accompanying license agreement).

You should refer to the license agreement accompanying this Software for
additional information regarding your rights and obligations.

SOFTWARE AND DOCUMENTATION ARE PROVIDED ?AS IS? WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF
MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE.
IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER
CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR
OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR
CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT OF
SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
(INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
*******************************************************************************/

#include <sys/types.h>
#include "net_tls_cyassl_glue.h"
#include "config.h"
#include <wolfssl/wolfcrypt/random.h>
#include <cyassl/ssl.h>
#include <cyassl/ctaocrypt/logging.h>
#include "ca-cert.h"

NET_TLS_ProviderData net_tls_CyaSSLProvider =
{
    .provOpen = NET_TLS_CyaSSLOpen,
    .provClose = NET_TLS_CyaSSLClose,
    .provStrClCr = NET_TLS_CyaSSLStreamClientCreate,
    .provStrClCon = NET_TLS_CyaSSLStreamClientConnect,
    .provStrSerCr = NET_TLS_CyaSSLStreamServerCreate,
    .provStrSerAcpt = NET_TLS_CyaSSLStreamServerAccept,
    .provDGClCr = NET_TLS_CyaSSLDataGramClientCreate,
    .provDGClCon = NET_TLS_CyaSSLDataGramClientConnect,
    .provDGSerCr = NET_TLS_CyaSSLDataGramServerCreate,
    .provDGSerAcpt = NET_TLS_CyaSSLDataGramServerAccept,
    .provConnClose = NET_TLS_CyaSSLConnectionClose,
    .provWr = NET_TLS_CyaSSLWrite,
    .provRd = NET_TLS_CyaSSLRead,
    .provPk = NET_TLS_CyaSSLPeek,
};

NET_TLS_TransportData * spTransData;
typedef struct
{
    void * ctx;
    void * ssl;
} NET_TLS_CyaSSLGlueProvData;


void NET_TLS_CG_Logging_cb(const int logLevel, const char *const logMessage)
{
    //TODO:  Put in the applications specific log function
}

int NET_TLS_CG_Stream_Receive_cb(void *sslin, char *buf, int sz, void *ctx)
{
    int fd = *(int *)ctx;
    uint16_t bufferSize;
    bufferSize = (*spTransData->fpStreamReadyToRead)((uintptr_t)fd);
    if (bufferSize == 0)
    {
        return CYASSL_CBIO_ERR_WANT_READ;
    }
    bufferSize = (*spTransData->fpStreamRead)((uintptr_t)fd, (uint8_t*)buf, sz);
    return bufferSize;
}

int NET_TLS_CG_Stream_Send_cb(void *sslin, char *buf, int sz, void *ctx)
{
    int fd = *(int *)ctx;
    uint16_t bufferSize;
    bufferSize = (*spTransData->fpStreamReadyToWrite)((uintptr_t)fd);
    if (bufferSize == 0)
    {
        return CYASSL_CBIO_ERR_WANT_WRITE;
    }

    bufferSize =  (*spTransData->fpStreamWrite)((uintptr_t)fd, (uint8_t*)buf, (uint16_t)sz);
    return bufferSize;
}

int NET_TLS_CG_Datagram_Receive_cb(void *sslin, char *buf, int sz, void *ctx)
{
    return -1;
}

int NET_TLS_CG_Datagram_Send_cb(void *sslin, char *buf, int sz, void *ctx)
{
    return -1;
}


bool NET_TLS_CyaSSLOpen(NET_TLS_TransportData * transData)
{
    CyaSSL_SetLoggingCb(&NET_TLS_CG_Logging_cb);
    CyaSSL_Debugging_ON();
    CyaSSL_Init();
    spTransData = transData;
    return true;

}

bool NET_TLS_CyaSSLClose()
{
    return CyaSSL_Cleanup() == SSL_SUCCESS;
}

bool NET_TLS_CyaSSLStreamClientCreate(uintptr_t transHandle, int8_t certIndex, void * providerData)
{
    NET_TLS_CyaSSLGlueProvData * pData = (NET_TLS_CyaSSLGlueProvData *)providerData;
    // Suggested Optimzation: you can move the creating of the contexts to
    // NET_TLS_CyaSSLOpen.  That will save on memory.  But it prevents running both
    // clients and servers at the same time since they need different contexts.
    // Also contexts are linked to certificates to us, so if you want to use
    // different certifictes for differnet connections (for example the mail
    // server uses a different certificate than the web server). you'll need
    // to use different contexts.
    pData->ctx = CyaSSL_CTX_new(CyaSSLv23_client_method());
    if (pData->ctx == 0)
    {
        return false;
    }
    CyaSSL_SetIORecv(pData->ctx, (CallbackIORecv)&NET_TLS_CG_Stream_Receive_cb);
    CyaSSL_SetIOSend(pData->ctx, (CallbackIOSend)&NET_TLS_CG_Stream_Send_cb);
    if (CyaSSL_CTX_load_verify_buffer(pData->ctx,
            caCert,
            caCert_len,
            SSL_FILETYPE_PEM) != SSL_SUCCESS)
    {
        // Couldn't load the certificates
        //SYS_CONSOLE_MESSAGE("Something went wrong loading the certificates\r\n");
        CyaSSL_CTX_free(pData->ctx);
        return false;
    }
    // Turn off verification, because SNTP is usually blocked by a firewall
    CyaSSL_CTX_set_verify(pData->ctx, SSL_VERIFY_NONE, 0);
    pData->ssl = CyaSSL_new(pData->ctx);
    if (pData->ssl == 0)
    {
        CyaSSL_CTX_free(pData->ctx);
        return false;
    }
     if (CyaSSL_set_fd(pData->ssl, transHandle) != SSL_SUCCESS)
     {
        CyaSSL_free(pData->ssl);
        CyaSSL_CTX_free(pData->ctx);
        return false;
     }
    return true;

}


NET_TLS_SessionStatus NET_TLS_CyaSSLStreamClientConnect(void * providerData)
{
    NET_TLS_CyaSSLGlueProvData * pData = (NET_TLS_CyaSSLGlueProvData *)providerData;
    int result = CyaSSL_connect(pData->ssl);
    switch (result)
    {
        case SSL_SUCCESS:
            return NET_TLS_SS_OPEN;
        default:
        {
            int error = CyaSSL_get_error(pData->ssl, result);
            switch (error)
            {
                case SSL_ERROR_WANT_READ:
                case SSL_ERROR_WANT_WRITE:
                    return NET_TLS_SS_CLIENT_NEGOTIATING;
                default:
                    return NET_TLS_SS_FAILED;
            }
        }
    }
    return NET_TLS_SS_FAILED;
}



bool NET_TLS_CyaSSLStreamServerCreate(uintptr_t transHandle, int8_t certIndex, void * providerData)
{
    return false;
}
NET_TLS_SessionStatus NET_TLS_CyaSSLStreamServerAccept(void * providerData)
{
    return NET_TLS_SS_FAILED;
}
bool NET_TLS_CyaSSLDataGramClientCreate(uintptr_t transHandle, int8_t certIndex, void * providerData)
{
    return false;
}


NET_TLS_SessionStatus NET_TLS_CyaSSLDataGramClientConnect(void * providerData)
{
    return NET_TLS_SS_FAILED;
}

bool NET_TLS_CyaSSLDataGramServerCreate(uintptr_t transHandle, int8_t certIndex, void * providerData)
{
    return false;

}

NET_TLS_SessionStatus NET_TLS_CyaSSLDataGramServerAccept(void * providerData)
{
    return NET_TLS_SS_FAILED;

}

NET_TLS_SessionStatus NET_TLS_CyaSSLConnectionClose(void * providerData)
{
    NET_TLS_CyaSSLGlueProvData * pData = (NET_TLS_CyaSSLGlueProvData *)providerData;
    CyaSSL_free(pData->ssl);
    CyaSSL_CTX_free(pData->ctx);
    return NET_TLS_SS_CLOSED;
}
int32_t NET_TLS_CyaSSLWrite(void * providerData, const uint8_t * buffer, uint16_t size)
{
    NET_TLS_CyaSSLGlueProvData * pData = (NET_TLS_CyaSSLGlueProvData *)providerData;
    int ret = CyaSSL_write(pData->ssl, buffer, size);
    if (ret < 0)
    {
        int error = CyaSSL_get_error(pData->ssl, ret);
        if ((error == SSL_ERROR_WANT_READ) ||
            (error == SSL_ERROR_WANT_WRITE))
        {
            return 0;
        }        
    }
    return ret;
}
int32_t NET_TLS_CyaSSLRead(void * providerData, uint8_t * buffer, uint16_t size)
{
    NET_TLS_CyaSSLGlueProvData * pData = (NET_TLS_CyaSSLGlueProvData *)providerData;
    int ret = CyaSSL_read(pData->ssl, buffer, size);
    if (ret < 0)
    {
        int error = CyaSSL_get_error(pData->ssl, ret);
        if ((error == SSL_ERROR_WANT_READ) ||
            (error == SSL_ERROR_WANT_WRITE))
        {
            return 0;
        }
    }
    return ret;

}

int32_t NET_TLS_CyaSSLPeek(void * providerData, uint8_t * buffer, uint16_t size)
{
    NET_TLS_CyaSSLGlueProvData * pData = (NET_TLS_CyaSSLGlueProvData *)providerData;
    int ret = CyaSSL_peek(pData->ssl, buffer, size);
    if (ret < 0)
    {
        int error = CyaSSL_get_error(pData->ssl, ret);
        if ((error == SSL_ERROR_WANT_READ) ||
            (error == SSL_ERROR_WANT_WRITE))
        {
            return 0;
        }
    }
    return ret;

}

CYASSL_API int  InitRng(RNG* in)
{
    return wc_InitRng(in);
}
