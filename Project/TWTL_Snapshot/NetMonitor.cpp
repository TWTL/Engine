#include "stdafx.h"
#include "NetMonitor.h"

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

char* __stdcall GetDomainName(uint32_t ip);
BOOL __stdcall isBlacklist(TWTL_DB_NETWORK* sqliteNet1, char szRemoteAddr[], CONST DWORD32 index, JSON_EnqTrapQueue_t trapProc, TWTL_TRAP_QUEUE* queue, sqlite3* db);

/*
	( in ) JSON_EnqTrapQueue_t trapProc
		Reserve a trap to be sent to GUI
	( in ) TWTL_TRAP_QUEUE* queue
		A queue to hold trap data
	( in ) sqlite3* db :
		Database context, must be opened by DB_Connect()
*/
BOOL __stdcall ParseNetstat(
	TWTL_DB_NETWORK* sqliteNet1, 
	TWTL_DB_NETWORK* sqliteNet2,
	DWORD structSize[],
	JSON_EnqTrapQueue_t trapProc,
	TWTL_TRAP_QUEUE* queue,
	sqlite3* db,
	CONST DWORD32 mode
)
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
#ifdef _DEBUG
		printf("Number of entries: %d\n", (int)pTcpTable->dwNumEntries);
#endif
		if (mode == 2) {
			structSize[6] = (int)pTcpTable->dwNumEntries;
		}
		for (i = 0; i < (int)pTcpTable->dwNumEntries; i++) {
			if (mode == 2) {
				break;
			}
#ifdef _DEBUG
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
#endif
			IpAddr.S_un.S_addr = (u_long)pTcpTable->table[i].dwLocalAddr;
			sqliteNet1[i].is_dangerous = 0;
			sqliteNet1[i].src_ipv4 = IpAddr.S_un.S_addr;
			sqliteNet1[i].src_port = ntohs((u_short)pTcpTable->table[i].dwLocalPort);
			strcpy_s(szLocalAddr, sizeof(szLocalAddr), inet_ntoa(IpAddr));
#ifdef _DEBUG
			printf("TCP[%d] Local Addr: %s", i, szLocalAddr);
			printf("TCP[%d] Local Addr: %lu\n", i, sqliteNet1[i].src_ipv4);
			printf("TCP[%d] Local Port: %d \n", i,
				sqliteNet1[i].src_port);
#endif
			IpAddr.S_un.S_addr = (u_long)pTcpTable->table[i].dwRemoteAddr;
			sqliteNet1[i].dest_ipv4 = IpAddr.S_un.S_addr;

			// strcpy_s(hostName, 1025, GetDomainName(sqliteNet1[i].dest_ipv4));

			sqliteNet1[i].dest_port = ntohs((u_short)pTcpTable->table[i].dwRemotePort);
			strcpy_s(szRemoteAddr, sizeof(szRemoteAddr), inet_ntoa(IpAddr));
			sqliteNet1[i].pid = (uint16_t)pTcpTable->table[i].dwOwningPid;
			isBlacklist(sqliteNet1, szRemoteAddr, i, trapProc, queue, db);
#ifdef _DEBUG
			printf("TCP[%d] Remote Addr: %s", i, szRemoteAddr);
			// printf("TCP[%d] Remote Addr: %lu\n", i, sqliteNet1[i].dest_ipv4);
			printf("TCP[%d] Remote Addr: %s\n", i, hostName);
			printf("TCP[%d] Remote Port: %d\n", i,
				sqliteNet1[i].dest_port);
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
#endif
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
#ifdef _DEBUG
		printf("Number of entries: %d\n", (int)pUdpTable->dwNumEntries);
#endif
		if (mode == 2) {
			structSize[7] = (int)pUdpTable->dwNumEntries;
		}
		for (i = 0; i < (int)pUdpTable->dwNumEntries; i++) {
			if (mode == 2) {
				break;
			}
			IpAddr.S_un.S_addr = (u_long)pUdpTable->table[i].dwLocalAddr;
			sqliteNet2[i].is_dangerous = 0;
			sqliteNet2[i].src_ipv4 = IpAddr.S_un.S_addr;
			sqliteNet2[i].src_port = ntohs((u_short)pUdpTable->table[i].dwLocalPort);
			strcpy_s(szLocalAddr, sizeof(szLocalAddr), inet_ntoa(IpAddr));
#ifdef _DEBUG
			printf("UDP[%d] Local Addr: %s\n", i, szLocalAddr);
			printf("UDP[%d] Local Port: %d\n", i,
				sqliteNet2[i].src_port);
#endif
			sqliteNet2[i].pid = (uint16_t) pUdpTable->table[i].dwOwningPid;
#ifdef _DEBUG
			printf("UDP[%d] Owning PID: %d\n\n", i, sqliteNet2[i].pid);
#endif

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
	static char hostname[NI_MAXHOST] = { 0 };
	char servInfo[NI_MAXSERV] = { 0 };
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

/*
	( in ) JSON_EnqTrapQueue_t trapProc
		Reserve a trap to be sent to GUI
	( in ) TWTL_TRAP_QUEUE* queue
		A queue to hold trap data
	( in ) sqlite3* db :
		Database context, must be opened by DB_Connect()
*/
BOOL __stdcall isBlacklist(TWTL_DB_NETWORK* sqliteNet1, char szRemoteAddr[], CONST DWORD32 index, JSON_EnqTrapQueue_t trapProc, TWTL_TRAP_QUEUE* queue, sqlite3* db) {
	FILE *f;
	fopen_s(&f, "Blacklist.dat", "r");
	strcat_s(szRemoteAddr, 128, "\n");
	TCHAR path[MAX_PATH] = { 0, };
	if (f != NULL) {
		char comparedIP[17] = { 0, };
		while (!feof(f))
		{
			fgets(comparedIP, sizeof(comparedIP), f);
			if (!strcmp(szRemoteAddr, comparedIP)) {
				sqliteNet1[index].is_dangerous = 1;
				printf("%d is dangerous!!! PID : %d, IP : %s", index, sqliteNet1[index].pid, szRemoteAddr);
				if (trapProc)
				{
					trapProc(queue, "/Net/Connections/");
				}

				
				int dbCount = 0;
				DB_Select(db, DB_BLACKLIST, NULL, &dbCount, NULL);
				TWTL_DB_BLACKLIST* dbBlack = (TWTL_DB_BLACKLIST*) calloc(dbCount + 1, sizeof(TWTL_DB_BLACKLIST));
				DB_Select(db, DB_BLACKLIST, dbBlack, &dbCount, NULL);
				TerminateCurrentProcess(0, NULL, dbBlack, dbCount, 1);
				free(dbBlack);
			}
		}
		fclose(f);
		return TRUE;
	}
	else {
#ifdef _DEBUG
		printf("Error Opening Blacklist Database.\n");
#endif	
		return NULL;
	}
}