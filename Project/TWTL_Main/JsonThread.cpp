#include "stdafx.h"

#include "JsonFunc.h"
#include "Socket.h"

extern BOOL g_runJsonMainThread;
extern BOOL g_runJsonTrapThread;
extern TWTL_TRAP_QUEUE trapQueue;
extern SHORT trapPort;

unsigned int WINAPI JsonMainThreadProc(LPVOID lpParam)
{
	JSON_Init_TWTL_INFO_DATA();

	SOCK_MainPortInit();

	if (SOCK_MainPortProc())
	{ // Error Handling
		fprintf(stderr, "SOCK_MainPortProc() failed\n\n");
	}

	SOCK_MainPortClose();

	return 0;
}

unsigned int WINAPI JsonTrapThreadProc(LPVOID lpParam)
{
	JSON_InitTrapQueue(&trapQueue);
	char trapPortBuf[TWTL_JSON_MAX_BUF];

	while (trapPort == 0)
	{
		DelayWait(1000);
	}

	StringCchPrintfA(trapPortBuf, TWTL_JSON_MAX_BUF, "%d", trapPort);
	SOCK_TrapPortInit("127.0.0.1", trapPortBuf);

	/*
	// Trap 
	TWTL_PROTO_BUF res;
	memset(&res, 0, sizeof(TWTL_PROTO_BUF));

	StringCchCopyA(res.app, TWTL_JSON_MAX_BUF, "TWTL-Engine");
	StringCchCopyA(res.name, TWTL_JSON_MAX_BUF, "TWTL");
	StringCchCopyA(res.version, TWTL_JSON_MAX_BUF, "1.0");
	TWTL_PROTO_NODE* node = JSON_AddProtoNode(&res);
	JSON_EnqTrapQueue(&trapQueue, &res);
	*/

	SOCK_TrapPortProc();
	SOCK_TrapPortClose();

	JSON_ClearTrapQueue(&trapQueue);

	return 0;
}

