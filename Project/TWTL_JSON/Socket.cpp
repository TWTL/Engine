#include "stdafx.h"

#include "TWTL_JSON.h"

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

TWTL_JSON_API DWORD __stdcall JSON_InitServerSocket(WSADATA* wsaData, SOCKET* clientSocket)
{
	int iResult;

	SOCKET listenSocket = INVALID_SOCKET;
	*clientSocket = INVALID_SOCKET;

	struct addrinfoW *result = NULL;
	struct addrinfoW hints;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), wsaData);
	if (iResult != 0) {
		fprintf(stderr, "WSAStartup failed with error: %d\n", iResult);
		return TRUE;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;			// IPv4
	hints.ai_socktype = SOCK_STREAM;	// TCP
	hints.ai_protocol = IPPROTO_TCP;	// TCP
	hints.ai_flags = AI_PASSIVE;        // Will be binded

	// Resolve the server address and port
	iResult = GetAddrInfoW(NULL, TWTL_JSON_PORT_WSTR, &hints, &result);
	if (iResult != 0) {
		fprintf(stderr, "getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return TRUE;
	}

	// Create a SOCKET for connecting to server
	listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (listenSocket == INVALID_SOCKET) {
		fprintf(stderr, "socket failed with error: %ld\n", WSAGetLastError());
		FreeAddrInfoW(result);
		WSACleanup();
		return TRUE;
	}

	// Setup the TCP listening socket
	iResult = bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		fprintf(stderr, "bind failed with error: %d\n", WSAGetLastError());
		FreeAddrInfoW(result);
		closesocket(listenSocket);
		WSACleanup();
		return TRUE;
	}

	FreeAddrInfoW(result);

	iResult = listen(listenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		fprintf(stderr, "listen failed with error: %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return TRUE;
	}

	BOOL bOptVal = TRUE;
	int bOptLen = sizeof(BOOL);

	// Set Socket Option
	iResult = setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&bOptVal, bOptLen);
	if (iResult == SOCKET_ERROR) {
		wprintf(L"setsockopt for SO_KEEPALIVE failed with error: %u\n", WSAGetLastError());
	}
	else
		wprintf(L"Set SO_KEEPALIVE: ON\n");

	// Accept a client socket
	*clientSocket = accept(listenSocket, NULL, NULL);
	if (*clientSocket == INVALID_SOCKET) {
		fprintf(stderr, "accept failed with error: %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return TRUE;
	}

	// No longer need server socket
	closesocket(listenSocket);

	return FALSE;
}

TWTL_JSON_API DWORD __stdcall JSON_ProcServerSocket(SOCKET* clientSocket)
{
	char recvbuf[TWTL_JSON_MAX_BUF];
	int iResult = 0;

	// Receive until the peer shuts down the connection
	do {
		iResult = recv(*clientSocket, recvbuf, TWTL_JSON_MAX_BUF, 0);
		if (iResult > 0) {
			fprintf(stderr, "Bytes received: %d\n", iResult);
			// iResult == recieved packet size

			printf("[Recv]\n%s\n", recvbuf);
			ParseJSON(recvbuf);
			/*
			// Echo the buffer back to the sender
			iSendResult = send(*clientSocket, recvbuf, iResult, 0);
			if (iSendResult == SOCKET_ERROR) {
				printf("send failed with error: %d\n", WSAGetLastError());
				closesocket(*clientSocket);
				WSACleanup();
				return TRUE;
			}
			fprintf(stderr, "Bytes sent: %d\n", iSendResult);
			*/
		}
		else if (iResult == 0)
			fprintf(stderr, "Connection closing...\n");
		else {
			fprintf(stderr, "recv failed with error: %d\n", WSAGetLastError());
			closesocket(*clientSocket);
			WSACleanup();
			return TRUE;
		}
	}
	while (iResult > 0);
	return FALSE;
}

TWTL_JSON_API DWORD __stdcall JSON_CloseServerSocket(SOCKET* clientSocket)
{
	// shutdown the connection since we're done
	int iResult = shutdown(*clientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		fprintf(stderr, "shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(*clientSocket);
		WSACleanup();
		return TRUE;
	}

	// cleanup
	closesocket(*clientSocket);
	WSACleanup();

	return FALSE;
}


TWTL_JSON_API DWORD __stdcall JSON_InitClientSocket(LPCWSTR address, LPCWSTR port, SOCKET* connectSocket)
{
	// Initialize Winsock
	WSADATA wsaData;
	struct addrinfoW *result = NULL, *ptr = NULL;
	struct addrinfoW hints;
	int iResult;

	*connectSocket = INVALID_SOCKET;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		fprintf(stderr, "WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = GetAddrInfoW(address, port, &hints, &result);
	if (iResult != 0) {
		fprintf(stderr, "getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return TRUE;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		*connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (*connectSocket == INVALID_SOCKET) {
			fprintf(stderr, "socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return TRUE;
		}

		// Connect to server.
		iResult = connect(*connectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(*connectSocket);
			*connectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	FreeAddrInfoW(result);

	if (*connectSocket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		return TRUE;
	}

	return FALSE;
}

TWTL_JSON_API DWORD __stdcall JSON_ProcClientSocket(SOCKET* connectSocket)
{
	char sendbuf[TWTL_JSON_MAX_BUF];

	StringCchCopyA(sendbuf, TWTL_JSON_MAX_BUF, "{\n    \"glossary\": {\n        \"title\": \"example glossary\",\n		\"GlossDiv\": {\n            \"title\": \"S\",\n			\"GlossList\": {\n                \"GlossEntry\": {\n                    \"ID\": \"SGML\",\n					\"SortAs\": \"SGML\",\n					\"GlossTerm\": \"Standard Generalized Markup Language\",\n					\"Acronym\": \"SGML\",\n					\"Abbrev\": \"ISO 8879:1986\",\n					\"GlossDef\": {\n                        \"para\": \"A meta-markup language, used to create markup languages such as DocBook.\",\n						\"GlossSeeAlso\": [\"GML\", \"XML\"]\n                    },\n					\"GlossSee\": \"markup\"\n                }\n            }\n        }\n    }\n}");
	printf("[Recv]\n%s\n", sendbuf);

	// Send an initial buffer
	int iResult = send(*connectSocket, sendbuf, (int) strlen(sendbuf) + 1, 0);
	if (iResult == SOCKET_ERROR) {
		fprintf(stderr, "send failed with error: %d\n", WSAGetLastError());
		closesocket(*connectSocket);
		WSACleanup();
		return TRUE;
	}

	printf("Bytes Sent: %ld\n", iResult);
	
	return FALSE;
}

TWTL_JSON_API DWORD __stdcall JSON_CloseClientSocket(SOCKET* connectSocket)
{
	// shutdown the connection since no more data will be sent
	int iResult = shutdown(*connectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(*connectSocket);
		WSACleanup();
		return TRUE;
	}
	// cleanup
	closesocket(*connectSocket);
	WSACleanup();

	return FALSE;
}
