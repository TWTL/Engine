#include "stdafx.h"

#include "../TWTL_JSON/TWTL_JSON.h"
#include "../TWTL_JSON/Socket.h"

extern BOOL g_runJsonMainThread;
extern BOOL g_runJsonTrapThread;

unsigned int WINAPI JsonMainThreadProc(LPVOID lpParam)
{
	JSON_Init_TWTL_INFO_DATA();

	JSON_InitMainSocket();
	while (g_runJsonMainThread)
	{
		JSON_ProcMainSocket(&g_runJsonMainThread);
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
		JSON_ProcTrapSocket(&g_runJsonTrapThread);
	}
	JSON_CloseTrapSocket();
	*/

	return 0;
}
