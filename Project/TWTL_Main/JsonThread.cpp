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

	FILE* fp = NULL;
	fopen_s(&fp, "json_log.txt", "w");
	fclose(fp);

	while (g_runJsonMainThread)
	{
		SOCK_MainPortInit();
		if (SOCK_MainPortProc())
		{ // Error Handling
			fprintf(stderr, "SOCK_MainPortProc() failed\n\n");
		}
		SOCK_MainPortClose();
		trapPort = 0;
	}

	printf("Json MainPortThread Terminated\n");

	return 0;
}

unsigned int WINAPI JsonTrapThreadProc(LPVOID lpParam)
{
	while (g_runJsonTrapThread)
	{
		JSON_InitTrapQueue(&trapQueue);
		char trapPortBuf[TWTL_JSON_MAX_BUF];

		trapPort = 0;
		
		while (trapPort == 0)
		{
			if (g_runJsonTrapThread == FALSE)
				break;
			DelayWait(1000);
		}

		if (g_runJsonTrapThread == FALSE)
			break;

		StringCchPrintfA(trapPortBuf, TWTL_JSON_MAX_BUF, "%d", trapPort);
		SOCK_TrapPortInit("127.0.0.1", trapPortBuf);

		SOCK_TrapPortProc();
		SOCK_TrapPortClose();

		JSON_ClearTrapQueue(&trapQueue);
	}

	printf("Json TrapPortThread Terminated\n");

	return 0;
}

