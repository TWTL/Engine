#include "stdafx.h"

#include "../TWTL_JSON/TWTL_JSON.h"
#include "../TWTL_JSON/Socket.h"

extern BOOL g_runJsonMainThread;
extern BOOL g_runJsonTrapThread;

unsigned int WINAPI JsonMainThreadProc(LPVOID lpParam)
{
	JSON_Init_TWTL_INFO_DATA();

	while (g_runJsonMainThread)
	{
		JSON_InitMainSocket();

		JSON_ProcMainSocket(&g_runJsonMainThread);

		JSON_CloseMainSocket();
	}

	return 0;
}

unsigned int WINAPI JsonTrapThreadProc(LPVOID lpParam)
{
	// TODO
	/*
	while (g_runJsonTrapThread)
	{
		JSON_InitTrapSocket("127.0.0.1", (LPCSTR)lpParam);

		JSON_ProcTrapSocket(&g_runJsonTrapThread);

		JSON_CloseTrapSocket();
	}
	*/

	return 0;
}
