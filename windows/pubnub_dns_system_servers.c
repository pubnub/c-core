/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#include "core/pubnub_dns_servers.h"
#include "lib/pubnub_parse_ipv4_addr.h"
#include "core/pubnub_assert.h"
#include "core/pubnub_log.h"

#include <winsock2.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <windows.h>


#pragma comment(lib, "IPHLPAPI.lib")


#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

int pubnub_dns_read_system_servers_ipv4(struct pubnub_ipv4_address* o_ipv4, size_t n)
{
    FIXED_INFO*     pFixedInfo;
    ULONG           ulOutBufLen;
    DWORD           dwRetVal;
    IP_ADDR_STRING* pIPAddr;
    unsigned        j;

    pFixedInfo = (FIXED_INFO*)MALLOC(sizeof(FIXED_INFO));
    if (pFixedInfo == NULL) {
        PUBNUB_LOG_ERROR(
            "Error allocating memory needed to call GetNetworkParams\n");
        return -1;
    }
    ulOutBufLen = sizeof(FIXED_INFO);

    // Make an initial call to GetAdaptersInfo to get
    // the necessary size into the ulOutBufLen variable
    if (GetNetworkParams(pFixedInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
        FREE(pFixedInfo);
        pFixedInfo = (FIXED_INFO*)MALLOC(ulOutBufLen);
        if (pFixedInfo == NULL) {
            PUBNUB_LOG_ERROR(
                "Error allocating memory needed to call GetNetworkParams\n");
            return -1;
        }
    }
    dwRetVal = GetNetworkParams(pFixedInfo, &ulOutBufLen);
    if (NO_ERROR == dwRetVal) {
        j       = 0;
        pIPAddr = &pFixedInfo->DnsServerList;
        while ((j < n) && pIPAddr) {
            struct pubnub_ipv4_address addr;
            if (pubnub_parse_ipv4_addr(pIPAddr->IpAddress.String, &addr) == 0) {
                memcpy(o_ipv4[j++].ipv4, addr.ipv4, sizeof o_ipv4[0].ipv4);
            }
            pIPAddr = pIPAddr->Next;
        }
    }
    else {
        PUBNUB_LOG_ERROR("GetNetworkParams failed with error: %d\n", dwRetVal);
        FREE(pFixedInfo);
        return -1;
    }

    FREE(pFixedInfo);

    return j;
}