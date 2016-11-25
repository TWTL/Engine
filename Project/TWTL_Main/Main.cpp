// Main.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "JsonThread.h"
#include "JsonFunc.h"

BOOL g_runJsonMainThread = FALSE;
BOOL g_runJsonTrapThread = FALSE;
TWTL_TRAP_QUEUE trapQueue;
SHORT trapPort;

int main()
{	
	HANDLE hJsonThread[2] = { INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE };

	DWORD select=NULL;
	DWORD32 targetKey = NULL;
	DWORD32 targetPID = NULL;
	TCHAR keyName[REGNAME_MAX] = { 0, };
	TCHAR keyValue[REGVALUE_MAX] = { 0, };

	SetPrivilege(SE_DEBUG_NAME, TRUE);

	hJsonThread[0] = (HANDLE)_beginthreadex(NULL, 0, &JsonMainThreadProc, (LPVOID)NULL, 0, NULL);
	hJsonThread[1] = (HANDLE)_beginthreadex(NULL, 0, &JsonTrapThreadProc, (LPVOID)NULL, 0, NULL);
	g_runJsonMainThread = TRUE;
	g_runJsonTrapThread = TRUE;
	wprintf_s(L"Json Threads running\n\n");

	while (TRUE) {
		SnapCurrentStatus(0);
		DelayWait(3000);
	}
	
	wprintf_s(L"\nTerminating Json Threads...\n");
	g_runJsonMainThread = FALSE;
	g_runJsonTrapThread = FALSE;
	WaitForMultipleObjects(2, hJsonThread, TRUE, INFINITE);
	CloseHandle(hJsonThread[0]);
	CloseHandle(hJsonThread[1]);

	wprintf_s(L"\nBye!\n");
	DelayWait(5000);
	
	return 0;
}

// Linux-Style Hex Dump, written by Joveler
void BinaryDump(const uint8_t buf[], const uint32_t bufsize)
{
	uint32_t base = 0;
	uint32_t interval = 16;
	while (base < bufsize)
	{
		if (base + 16 < bufsize)
			interval = 16;
		else
			interval = bufsize - base;

		printf("0x%04x:   ", base);
		for (uint32_t i = base; i < base + 16; i++) // i for dump
		{
			if (i < base + interval)
				printf("%02x", buf[i]);
			else
			{
				putchar(' ');
				putchar(' ');
			}

			if ((i + 1) % 2 == 0)
				putchar(' ');
			if ((i + 1) % 8 == 0)
				putchar(' ');
		}
		putchar(' ');
		putchar(' ');
		for (uint32_t i = base; i < base + 16; i++) // i for dump
		{
			if (i < base + interval)
			{
				if (0x20 <= buf[i] && buf[i] <= 0x7E)
					printf("%c", buf[i]);
				else
					putchar('.');
			}
			else
			{
				putchar(' ');
				putchar(' ');
			}

			if ((i + 1) % 8 == 0)
				putchar(' ');
		}
		putchar('\n');


		if (base + 16 < bufsize)
			base += 16;
		else
			base = bufsize;
	}

	return;
}