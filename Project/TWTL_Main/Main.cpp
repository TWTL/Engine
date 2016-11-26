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

	TWTL_DB_PROCESS*  sqlitePrc;
	TWTL_DB_REGISTRY* sqliteReg1;
	TWTL_DB_REGISTRY* sqliteReg2;
	TWTL_DB_REGISTRY* sqliteReg3;
	TWTL_DB_REGISTRY* sqliteReg4;
	TWTL_DB_SERVICE*  sqliteSvc;
	TWTL_DB_NETWORK*  sqliteNet1;
	TWTL_DB_NETWORK*  sqliteNet2;

	SetPrivilege(SE_DEBUG_NAME, TRUE);

	hJsonThread[0] = (HANDLE)_beginthreadex(NULL, 0, &JsonMainThreadProc, (LPVOID)NULL, 0, NULL);
	hJsonThread[1] = (HANDLE)_beginthreadex(NULL, 0, &JsonTrapThreadProc, (LPVOID)NULL, 0, NULL);
	g_runJsonMainThread = TRUE;
	g_runJsonTrapThread = TRUE;
	DB_Connect(L"TWTL_Database.db");
	wprintf_s(L"Json Threads running\n\n");

	while (TRUE) {
		sqlitePrc = (TWTL_DB_PROCESS*)calloc(100, sizeof(TWTL_DB_PROCESS));
		sqliteReg1 = (TWTL_DB_REGISTRY*)calloc(200, sizeof(TWTL_DB_REGISTRY));
		sqliteReg2 = (TWTL_DB_REGISTRY*)calloc(200, sizeof(TWTL_DB_REGISTRY));
		sqliteReg3 = (TWTL_DB_REGISTRY*)calloc(200, sizeof(TWTL_DB_REGISTRY));
		sqliteReg4 = (TWTL_DB_REGISTRY*)calloc(200, sizeof(TWTL_DB_REGISTRY));
		sqliteSvc = (TWTL_DB_SERVICE*)calloc(600, sizeof(TWTL_DB_SERVICE));
		sqliteNet1 = (TWTL_DB_NETWORK*)calloc(300, sizeof(TWTL_DB_NETWORK));
		sqliteNet2 = (TWTL_DB_NETWORK*)calloc(300, sizeof(TWTL_DB_NETWORK));
		
		SnapCurrentStatus(
			sqlitePrc, 
			sqliteReg1, 
			sqliteReg2, 
			sqliteReg3, 
			sqliteReg4,
			sqliteSvc, 
			sqliteNet1,
			sqliteNet2,
			0);


		DelayWait(5000);
		free(sqlitePrc);
		free(sqliteReg1);
		free(sqliteReg2);
		free(sqliteReg3);
		free(sqliteReg4);
		free(sqliteSvc);
		free(sqliteNet1);
		free(sqliteNet2);
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