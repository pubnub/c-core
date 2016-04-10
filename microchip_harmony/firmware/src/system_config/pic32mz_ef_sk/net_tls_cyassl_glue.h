/*******************************************************************************
 Header file for the CyaSSL glue functions to work with Harmony


  Summary:


  Description:

*******************************************************************************/

/*******************************************************************************
File Name: net_tls_cyassl_glue.h
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

#ifndef _NET_TLS_CYASSL_GLUE_H_
#define _NET_TLS_CYASSL_GLUE_H_

#include "system_config.h"
#include "net/pres/tls/net_tls.h"

extern NET_TLS_ProviderData net_tls_CyaSSLProvider;

bool NET_TLS_CyaSSLOpen(NET_TLS_TransportData * transData); // Called when the first TLS connection needs to be created
bool NET_TLS_CyaSSLClose();  // Called when TCPIP stack is deinitialized
bool NET_TLS_CyaSSLStreamClientCreate(uintptr_t transHandle, int8_t certIndex, void * providerData);  // Called after a TCP connection is open, allocates the ctx and ssl
NET_TLS_SessionStatus NET_TLS_CyaSSLStreamClientConnect(void * providerData); // called to pump the negotiation
bool NET_TLS_CyaSSLStreamServerCreate(uintptr_t transHandle, int8_t certIndex, void * providerData);  // Called after a TCP connection has been accepted, allocates the ctx and ssl
NET_TLS_SessionStatus NET_TLS_CyaSSLStreamServerAccept(void * providerData);  // Called to pump the negotiation
bool NET_TLS_CyaSSLDataGramClientCreate(uintptr_t transHandle, int8_t certIndex, void * providerData);  // Called after a TCP connection is open, allocates the ctx and ssl
NET_TLS_SessionStatus NET_TLS_CyaSSLDataGramClientConnect(void * providerData); // called to pump the negotiation
bool NET_TLS_CyaSSLDataGramServerCreate(uintptr_t transHandle, int8_t certIndex, void * providerData);  // Called after a TCP connection has been accepted, allocates the ctx and ssl
NET_TLS_SessionStatus NET_TLS_CyaSSLDataGramServerAccept(void * providerData);  // Called to pump the negotiation
NET_TLS_SessionStatus NET_TLS_CyaSSLConnectionClose(void * providerData); // starts closing the connection, pump the connection until closed.  When connection is closed ctx and ssl are freed from this function
int32_t NET_TLS_CyaSSLWrite(void * providerData, const uint8_t * buffer, uint16_t size);   // Sends data on the secure channel
int32_t NET_TLS_CyaSSLRead(void * providerData, uint8_t * buffer, uint16_t size);   // Receives data from the secure channel
int32_t NET_TLS_CyaSSLPeek(void * providerData, uint8_t * buffer, uint16_t size);   // Receives data from the secure channel



#endif