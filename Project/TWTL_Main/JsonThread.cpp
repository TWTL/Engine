#include "stdafx.h"

#include "JsonFunc.h"
#include "Socket.h"

extern BOOL g_runJsonMainThread;
extern BOOL g_runJsonTrapThread;

unsigned int WINAPI JsonMainThreadProc(LPVOID lpParam)
{
	JSON_Init_TWTL_INFO_DATA();

	SOCK_MainPortInit();

	if (SOCK_MainPortProc())
	{ // Error Handling
	}

	SOCK_MainPortClose();

	return 0;
}

unsigned int WINAPI JsonTrapThreadProc(LPVOID lpParam)
{
	SOCK_TrapPortInit("127.0.0.1", (LPCSTR)lpParam);
	while (g_runJsonTrapThread)
	{ // Error Handling
		DelayWait(1000);
	}
	SOCK_TrapPortClose();

	return 0;
}

