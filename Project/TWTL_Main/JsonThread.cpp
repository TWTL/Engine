#include "stdafx.h"

#include "JsonFunc.h"
#include "Socket.h"

extern BOOL g_runJsonMainThread;
extern BOOL g_runJsonTrapThread;

unsigned int WINAPI JsonMainThreadProc(LPVOID lpParam)
{
	JSON_Init_TWTL_INFO_DATA();

	JSON_InitMainSocket();
	while (g_runJsonMainThread)
	{
		if (JSON_ProcMainSocket(&g_runJsonMainThread))
		{ // Error Handling
			break;
		}
	}
	JSON_CloseMainSocket();

	return 0;
}

unsigned int WINAPI JsonTrapThreadProc(LPVOID lpParam)
{
	// TODO
	/*
	JSON_InitTrapSocket("127.0.0.1", (LPCSTR)lpParam);
	while (g_runJsonTrapThread)
	{
		if (JSON_ProcTrapSocket(&g_runJsonTrapThread))
		{ // Error Handling
			break;
		}
	}
	JSON_CloseTrapSocket();
	*/

	return 0;
}

