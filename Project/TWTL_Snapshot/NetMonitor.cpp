#include "stdafx.h"
#include "NetMonitor.h"

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

char* __stdcall GetDomainName(uint32_t ip);

BOOL __stdcall ParseNetstat(TWTL_DB_NETWORK* sqliteNet1, TWTL_DB_NETWORK* sqliteNet2)
{
	// Declare and initialize variables
	PMIB_UDPTABLE_OWNER_PID pUdpTable;
	PMIB_TCPTABLE2 pTcpTable;
	ULONG ulSize = 0;
	DWORD dwRetVal = 0;

	char szLocalAddr[128];
	char szRemoteAddr[128];
	char hostName[1025] = { 0, };

	struct in_addr IpAddr;

	int i;
	int total_entry = 0;

	pTcpTable = (MIB_TCPTABLE2 *)MALLOC(sizeof(MIB_TCPTABLE2));
	if (pTcpTable == NULL) {
		printf("Error allocating memory\n");
		return 1;
	}
	pUdpTable = (MIB_UDPTABLE_OWNER_PID *)MALLOC(sizeof(MIB_UDPTABLE_OWNER_PID));
	if (pTcpTable == NULL) {
		printf("Error allocating memory\n");
		return 1;
	}

	ulSize = sizeof(MIB_TCPTABLE);
	if ((dwRetVal = GetTcpTable2(pTcpTable, &ulSize, TRUE)) ==
		ERROR_INSUFFICIENT_BUFFER) {
		FREE(pTcpTable);
		pTcpTable = (MIB_TCPTABLE2 *)MALLOC(ulSize);
		if (pTcpTable == NULL) {
			printf("Error allocating memory\n");
			return 1;
		}
	}
	if ((dwRetVal = GetTcpTable2(pTcpTable, &ulSize, TRUE)) == NO_ERROR) {
		printf("Number of entries: %d\n", (int)pTcpTable->dwNumEntries);
		sqliteNet1 = (TWTL_DB_NETWORK*)realloc(sqliteNet1, sizeof(TWTL_DB_NETWORK)*((int)pTcpTable->dwNumEntries + 10));
		memset(sqliteNet1, 0x00, sizeof(TWTL_DB_NETWORK)*((int)pTcpTable->dwNumEntries + 10));
		for (i = 0; i < (int)pTcpTable->dwNumEntries; i++) {
			printf("\nTCP[%d] State: %ld - ", i,
				pTcpTable->table[i].dwState);
			switch (pTcpTable->table[i].dwState) {
			case MIB_TCP_STATE_CLOSED:
				printf("CLOSED\n");
				break;
			case MIB_TCP_STATE_LISTEN:
				printf("LISTEN\n");
				break;
			case MIB_TCP_STATE_SYN_SENT:
				printf("SYN-SENT\n");
				break;
			case MIB_TCP_STATE_SYN_RCVD:
				printf("SYN-RECEIVED\n");
				break;
			case MIB_TCP_STATE_ESTAB:
				printf("ESTABLISHED\n");
				break;
			case MIB_TCP_STATE_FIN_WAIT1:
				printf("FIN-WAIT-1\n");
				break;
			case MIB_TCP_STATE_FIN_WAIT2:
				printf("FIN-WAIT-2 \n");
				break;
			case MIB_TCP_STATE_CLOSE_WAIT:
				printf("CLOSE-WAIT\n");
				break;
			case MIB_TCP_STATE_CLOSING:
				printf("CLOSING\n");
				break;
			case MIB_TCP_STATE_LAST_ACK:
				printf("LAST-ACK\n");
				break;
			case MIB_TCP_STATE_TIME_WAIT:
				printf("TIME-WAIT\n");
				break;
			case MIB_TCP_STATE_DELETE_TCB:
				printf("DELETE-TCB\n");
				break;
			default:
				printf("UNKNOWN dwState value\n");
				break;
			}
			sqliteNet1[i].time = time(0);
			IpAddr.S_un.S_addr = (u_long)pTcpTable->table[i].dwLocalAddr;
			sqliteNet1[i].src_ipv4 = IpAddr.S_un.S_addr;
			sqliteNet1[i].src_port = ntohs((u_short)pTcpTable->table[i].dwLocalPort);
			strcpy_s(szLocalAddr, sizeof(szLocalAddr), inet_ntoa(IpAddr));
			printf("TCP[%d] Local Addr: %s\n", i, szLocalAddr);
			printf("TCP[%d] Local Addr: %lu\n", i, sqliteNet1[i].src_ipv4);
			printf("TCP[%d] Local Port: %d \n", i,
				sqliteNet1[i].src_port);

			IpAddr.S_un.S_addr = (u_long)pTcpTable->table[i].dwRemoteAddr;
			sqliteNet1[i].dest_ipv4 = IpAddr.S_un.S_addr;

			// strcpy_s(hostName, 1025, GetDomainName(sqliteNet1[i].dest_ipv4));

			sqliteNet1[i].dest_port = ntohs((u_short)pTcpTable->table[i].dwRemotePort);
			strcpy_s(szRemoteAddr, sizeof(szRemoteAddr), inet_ntoa(IpAddr));
			printf("TCP[%d] Remote Addr: %s\n", i, szRemoteAddr);
			// printf("TCP[%d] Remote Addr: %lu\n", i, sqliteNet1[i].dest_ipv4);
			printf("TCP[%d] Remote Addr: %s\n", i, hostName);
			printf("TCP[%d] Remote Port: %d\n", i,
				sqliteNet1[i].dest_port);

			sqliteNet1[i].pid = (uint16_t) pTcpTable->table[i].dwOwningPid;
			printf("TCP[%d] Owning PID: %d\n", i, sqliteNet1[i].pid);
			printf("TCP[%d] Offload State: %ld - ", i,
				pTcpTable->table[i].dwOffloadState);
			switch (pTcpTable->table[i].dwOffloadState) {
			case TcpConnectionOffloadStateInHost:
				printf("Owned by the network stack and not offloaded \n");
				break;
			case TcpConnectionOffloadStateOffloading:
				printf("In the process of being offloaded\n");
				break;
			case TcpConnectionOffloadStateOffloaded:
				printf("Offloaded to the network interface control\n");
				break;
			case TcpConnectionOffloadStateUploading:
				printf("In the process of being uploaded back to the network stack \n");
				break;
			default:
				printf("UNKNOWN Offload state value\n");
				break;
			}

		}
	}
	else {
		printf("GetTcpTable2 failed with %d\n", dwRetVal);
		FREE(pTcpTable);
		return 1;
	}

	if (pTcpTable != NULL) {
		FREE(pTcpTable);
		pTcpTable = NULL;
	}

	ulSize = sizeof(MIB_UDPTABLE_OWNER_PID);
	if ((dwRetVal = GetExtendedUdpTable(pUdpTable, &ulSize, TRUE, AF_INET, UDP_TABLE_OWNER_PID, NULL)) ==
		ERROR_INSUFFICIENT_BUFFER) {
		FREE(pUdpTable);
		pUdpTable = (MIB_UDPTABLE_OWNER_PID *)MALLOC(ulSize);
		if (pUdpTable == NULL) {
			printf("Error allocating memory\n");
			return 1;
		}
	}
	if ((dwRetVal = GetExtendedUdpTable(pUdpTable, &ulSize, TRUE, AF_INET, UDP_TABLE_OWNER_PID, NULL)) == NO_ERROR) {
		printf("Number of entries: %d\n", (int)pUdpTable->dwNumEntries);
		sqliteNet2 = (TWTL_DB_NETWORK*)realloc(sqliteNet2, sizeof(TWTL_DB_NETWORK)*((int)pUdpTable->dwNumEntries + 10));
		memset(sqliteNet2, 0x00, sizeof(TWTL_DB_NETWORK)*((int)pUdpTable->dwNumEntries + 10));
		for (i = 0; i < (int)pUdpTable->dwNumEntries; i++) {
			sqliteNet2[i].time = time(0);
			IpAddr.S_un.S_addr = (u_long)pUdpTable->table[i].dwLocalAddr;
			sqliteNet2[i].src_ipv4 = IpAddr.S_un.S_addr;
			sqliteNet2[i].src_port = ntohs((u_short)pUdpTable->table[i].dwLocalPort);
			strcpy_s(szLocalAddr, sizeof(szLocalAddr), inet_ntoa(IpAddr));
			printf("UDP[%d] Local Addr: %s\n", i, szLocalAddr);
			printf("UDP[%d] Local Port: %d\n", i,
				sqliteNet2[i].src_port);

			sqliteNet2[i].pid = (uint16_t) pUdpTable->table[i].dwOwningPid;
			printf("UDP[%d] Owning PID: %d\n\n", i, sqliteNet2[i].pid);

		}
	}
	else {
		printf("GetExtendedUdpTable failed with %d\n", dwRetVal);
		FREE(pUdpTable);
		return 1;
	}

	if (pUdpTable != NULL) {
		FREE(pUdpTable);
		pUdpTable = NULL;
	}

	return 0;
}

char* __stdcall GetDomainName(uint32_t ip) {
	WSADATA wsaData = { 0 };
	int iResult = 0;

	DWORD dwRetval;

	struct sockaddr_in saGNI;
	char hostname[NI_MAXHOST];
	char servInfo[NI_MAXSERV];
	u_short port = 27015;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return NULL;
	}
	
	saGNI.sin_family = AF_INET;
	saGNI.sin_addr.s_addr = ip;
	saGNI.sin_port = htons(port);

	dwRetval = getnameinfo((struct sockaddr *) &saGNI,
		sizeof(struct sockaddr),
		hostname,
		NI_MAXHOST, servInfo, NI_MAXSERV, NI_NUMERICSERV);

	if (dwRetval != 0) {
		printf("getnameinfo failed with error # %ld\n", WSAGetLastError());
		return NULL;
	}
	else {
		printf("getnameinfo returned hostname = %s\n", hostname);
		return hostname;
	}
}