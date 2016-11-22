#include "stdafx.h"

#include "Socket.h"
#include "JsonFunc.h"

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

DWORD __stdcall SOCK_MainPortResponse(TWTL_PROTO_BUF *req);

static SOCKET mainSocket;
static SOCKET trapSocket;

extern BOOL g_runJsonMainThread;
extern BOOL g_runJsonTrapThread;

DWORD __stdcall SOCK_MainPortInit()
{
	int iResult;

	WSADATA wsaData;
	SOCKET listenSocket = INVALID_SOCKET;
	mainSocket = INVALID_SOCKET;

	struct addrinfoW *result = NULL;
	struct addrinfoW hints;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
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

	// Accept a client socket
	mainSocket = accept(listenSocket, NULL, NULL);
	if (mainSocket == INVALID_SOCKET) {
		fprintf(stderr, "accept failed with error: %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return TRUE;
	}

	// No longer need server socket
	closesocket(listenSocket);

	// Set Recv-Socket Option
	DWORD timeOut = 1000;
	bOptLen = sizeof(DWORD);
	iResult = setsockopt(mainSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeOut, bOptLen);
	if (iResult == SOCKET_ERROR) {
		wprintf(L"setsockopt for SO_RCVTIMEO failed with error: %u\n", WSAGetLastError());
	}

	return FALSE;
}

DWORD __stdcall SOCK_MainPortProc()
{
	char recvbuf[TWTL_JSON_MAX_BUF];
	int iResult = 0;
	TWTL_PROTO_BUF* req = (TWTL_PROTO_BUF*)malloc(sizeof(TWTL_PROTO_BUF));
	memset(req, 0, sizeof(TWTL_PROTO_BUF));

	// Receive until the peer shuts down the connection
	while (1) {
		iResult = recv(mainSocket, recvbuf, TWTL_JSON_MAX_BUF, 0);
		if (iResult > 0) {
			fprintf(stderr, "Bytes received: %d\n", iResult);
			// iResult == recieved packet size

			if (JSON_Parse(recvbuf, iResult, req))
			{ // Error Handling
				fprintf(stderr, "JSON_Parse() failed\n");
			}
			else
			{
				// Make Response and send it!
				SOCK_MainPortResponse(req);
				
				// Clear ProtoBuf
				JSON_ClearProtoNode(req);
			}
		}
		else if (iResult == 0) {
			fprintf(stderr, "Connection closing...\n");
			if (!g_runJsonMainThread)
				break;
		}
		else {
			int errorCode = WSAGetLastError();
			if (errorCode != WSAETIMEDOUT)
			{
				fprintf(stderr, "recv failed with error: %d\n", errorCode);
				closesocket(mainSocket);
				WSACleanup();
				JSON_ClearProtoNode(req);
				free(req);
				return TRUE;
			}
		}
	};

	JSON_ClearProtoNode(req);
	free(req);

	return FALSE;
}

static DWORD __stdcall SOCK_MainPortResponse(TWTL_PROTO_BUF *req)
{
	TWTL_PROTO_BUF* res = (TWTL_PROTO_BUF*)malloc(sizeof(TWTL_PROTO_BUF));
	memset(res, 0, sizeof(TWTL_PROTO_BUF));

	JSON_ProtoMakeResponse(req, res);

	json_t* json_res = JSON_ProtoBufToJson(res);
	char* sendbuf = json_dumps(json_res, 0);
	JSON_ClearProtoNode(req);
	free(res);
	json_decref(json_res);

	printf("[Send]\n%s\n", sendbuf);

	int iResult = send(mainSocket, sendbuf, strlen(sendbuf) + 1, 0);
	free(sendbuf);
	if (iResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(mainSocket);
		WSACleanup();
		return TRUE;
	}
	return FALSE;
}

 DWORD __stdcall SOCK_MainPortClose()
{
	// shutdown the connection since we're done
	int iResult = shutdown(mainSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		fprintf(stderr, "shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(mainSocket);
		WSACleanup();
		return TRUE;
	}

	// cleanup
	closesocket(mainSocket);
	WSACleanup();

	return FALSE;
}


 DWORD __stdcall SOCK_TrapPortInit(LPCSTR address, LPCSTR port)
{
	// Initialize Winsock
	WSADATA wsaData;
	struct addrinfo *result = NULL, *ptr = NULL;
	struct addrinfo hints;
	int iResult;

	trapSocket = INVALID_SOCKET;

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
	iResult = getaddrinfo(address, port, &hints, &result);
	if (iResult != 0) {
		fprintf(stderr, "getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return TRUE;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		trapSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (trapSocket == INVALID_SOCKET) {
			fprintf(stderr, "socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return TRUE;
		}

		// Connect to server.
		iResult = connect(trapSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(trapSocket);
			trapSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (trapSocket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		return TRUE;
	}

	// Set Recv-Socket Option
	DWORD timeOut = 1000;
	int bOptLen = sizeof(DWORD);
	iResult = setsockopt(trapSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeOut, bOptLen);
	if (iResult == SOCKET_ERROR) {
		wprintf(L"setsockopt for SO_RCVTIMEO failed with error: %u\n", WSAGetLastError());
	}

	return FALSE;
}

 DWORD __stdcall SOCK_TrapPortSend()
 {
	 char sendbuf[TWTL_JSON_MAX_BUF];
	 char recvbuf[TWTL_JSON_MAX_BUF];

	 StringCchCopyA(sendbuf, TWTL_JSON_MAX_BUF, "{\n    \"name\": \"TWTL\",\n    \"app\": \"TWTL-GUI\",\n    \"version\": \"1\",\n    \"contents\": [\n        {\n            \"type\": \"request.get\",\n            \"path\": \"/Engine/Name/\"\n        },\n        {\n            \"type\": \"request.get\",\n            \"path\": \"/Engine/Version/\"\n        },\n        {\n            \"type\": \"request.get\",\n            \"path\": \"/Engine/RequestPort/\"\n        },\n        {\n            \"type\": \"request.set\",\n            \"path\": \"/Engine/TrapPort/\",\n            \"value\": 48207\n        }\n    ]\n}");
	 printf("[Recv]\n%s\n", sendbuf);

	 // Send an initial buffer
	 int iResult = send(trapSocket, sendbuf, (int)strlen(sendbuf) + 1, 0);
	 if (iResult == SOCKET_ERROR) {
		 fprintf(stderr, "send failed with error: %d\n", WSAGetLastError());
		 closesocket(trapSocket);
		 WSACleanup();
		 return TRUE;
	 }

	 TWTL_PROTO_BUF* req = (TWTL_PROTO_BUF*)malloc(sizeof(TWTL_PROTO_BUF));
	 memset(req, 0, sizeof(TWTL_PROTO_BUF));

	 // Receive until the peer shuts down the connection
	 while (1) {
		 iResult = recv(mainSocket, recvbuf, TWTL_JSON_MAX_BUF, 0);
		 if (iResult > 0) {
			 fprintf(stderr, "Bytes received: %d\n", iResult);
			 // iResult == recieved packet size

			 if (JSON_Parse(recvbuf, iResult, req))
			 { // Error Handling
				 fprintf(stderr, "JSON_Parse() failed\n");
			 }
			 else
			 {
				 // Make Response and send it!
				 SOCK_MainPortResponse(req);

				 // Clear ProtoBuf
				 JSON_ClearProtoNode(req);
			 }
		 }
		 else if (iResult == 0) {
			 fprintf(stderr, "Connection closing...\n");
			 if (!g_runJsonTrapThread)
				 break;
		 }
		 else {
			 int errorCode = WSAGetLastError();
			 if (errorCode != WSAETIMEDOUT)
			 {
				 fprintf(stderr, "recv failed with error: %d\n", errorCode);
				 closesocket(mainSocket);
				 WSACleanup();
				 JSON_ClearProtoNode(req);
				 free(req);
				 return TRUE;
			 }
		 }
	 };

	 JSON_ClearProtoNode(req);
	 free(req);

	 return FALSE;
 }

 DWORD __stdcall JSON_ProcTrapSocket(BOOL* quitSignal)
{
	char sendbuf[TWTL_JSON_MAX_BUF];
	char recvbuf[TWTL_JSON_MAX_BUF];

	StringCchCopyA(sendbuf, TWTL_JSON_MAX_BUF, "{\n    \"name\": \"TWTL\",\n    \"app\": \"TWTL-GUI\",\n    \"version\": \"1\",\n    \"contents\": [\n        {\n            \"type\": \"request.get\",\n            \"path\": \"/Engine/Name/\"\n        },\n        {\n            \"type\": \"request.get\",\n            \"path\": \"/Engine/Version/\"\n        },\n        {\n            \"type\": \"request.get\",\n            \"path\": \"/Engine/RequestPort/\"\n        },\n        {\n            \"type\": \"request.set\",\n            \"path\": \"/Engine/TrapPort/\",\n            \"value\": 48207\n        }\n    ]\n}");
	printf("[Recv]\n%s\n", sendbuf);

	// Send an initial buffer
	int iResult = send(trapSocket, sendbuf, (int) strlen(sendbuf) + 1, 0);
	if (iResult == SOCKET_ERROR) {
		fprintf(stderr, "send failed with error: %d\n", WSAGetLastError());
		closesocket(trapSocket);
		WSACleanup();
		return TRUE;
	}

	// Wait Trap-ACK
	iResult = recv(trapSocket, recvbuf, TWTL_JSON_MAX_BUF, 0);
	if (iResult > 0) {
		TWTL_PROTO_BUF req;

		fprintf(stderr, "Bytes received: %d\n", iResult);
		// iResult == recieved packet size

		printf("[Recv]\n%s\n", recvbuf);
		JSON_Parse(recvbuf, iResult, &req);

		// Make Response and send it!
		SOCK_MainPortResponse(&req);

		// Clear ProtoBuf
		JSON_ClearProtoNode(&req);
	}
	else if (iResult == 0)
		fprintf(stderr, "Connection closing...\n");
	else {
		int errorCode = WSAGetLastError();
		if (errorCode != WSAETIMEDOUT)
		{
			fprintf(stderr, "recv failed with error: %d\n", errorCode);
			closesocket(mainSocket);
			WSACleanup();
			return TRUE;
		}
	}

	printf("Bytes Sent: %ld\n", iResult);
	
	return FALSE;
}

 DWORD __stdcall SOCK_TrapPortClose()
{
	// shutdown the connection since no more data will be sent
	int iResult = shutdown(trapSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(trapSocket);
		WSACleanup();
		return TRUE;
	}
	// cleanup
	closesocket(trapSocket);
	WSACleanup();

	return FALSE;
}
