// Main.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <conio.h>

#include "JsonThread.h"
#include "JsonFunc.h"

void DiffReg(TWTL_DB_REGISTRY* nowHkcuRun, TWTL_DB_REGISTRY* nowHklmRun, TWTL_DB_REGISTRY* nowHkcuRunOnce, TWTL_DB_REGISTRY* nowHklmRunOnce, TWTL_DB_SERVICE* nowServices, DWORD nowSize[]);

TWTL_INFO_DATA g_twtlInfo;
BOOL g_runJsonMainThread = FALSE;
BOOL g_runJsonTrapThread = FALSE;
TWTL_TRAP_QUEUE trapQueue;
SHORT trapPort;
sqlite3 *g_db;
BOOL g_dbLock;

int main()
{	
	HANDLE hJsonThread[2] = { INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE };

	DWORD select=NULL;
	DWORD32 targetKey = NULL;
	DWORD32 targetPID = NULL;
	TCHAR keyName[REGNAME_MAX] = { 0, };
	TCHAR keyValue[REGVALUE_MAX] = { 0, };

	g_dbLock = FALSE;

	TWTL_DB_PROCESS*  sqlitePrc = (TWTL_DB_PROCESS*)calloc(1, sizeof(TWTL_DB_PROCESS));
	TWTL_DB_REGISTRY* sqliteReg1= (TWTL_DB_REGISTRY*)calloc(1, sizeof(TWTL_DB_REGISTRY));
	TWTL_DB_REGISTRY* sqliteReg2= (TWTL_DB_REGISTRY*)calloc(1, sizeof(TWTL_DB_REGISTRY));
	TWTL_DB_REGISTRY* sqliteReg3= (TWTL_DB_REGISTRY*)calloc(1, sizeof(TWTL_DB_REGISTRY));
	TWTL_DB_REGISTRY* sqliteReg4= (TWTL_DB_REGISTRY*)calloc(1, sizeof(TWTL_DB_REGISTRY));
	TWTL_DB_SERVICE*  sqliteSvc = (TWTL_DB_SERVICE*)calloc(1, sizeof(TWTL_DB_SERVICE));
	TWTL_DB_NETWORK*  sqliteNet1= (TWTL_DB_NETWORK*)calloc(1, sizeof(TWTL_DB_NETWORK));
	TWTL_DB_NETWORK*  sqliteNet2= (TWTL_DB_NETWORK*)calloc(1, sizeof(TWTL_DB_NETWORK));
	
	DWORD structSize[8] = { 0, };

	// 0 : Accessable, 1 : Can't Access
	DWORD lock = 1;

	// Initialization
	DWORD initSizeCheck = 0;

	SetPrivilege(SE_DEBUG_NAME, TRUE);

	// Init Database
	g_db = DB_Connect(L"TWTL_Database.db");
	DB_CreateTable(g_db, DB_PROCESS);
	DB_CreateTable(g_db, DB_REG_HKLM_RUN);
	DB_CreateTable(g_db, DB_REG_HKLM_RUNONCE);
	DB_CreateTable(g_db, DB_REG_HKCU_RUN);
	DB_CreateTable(g_db, DB_REG_HKCU_RUNONCE);
	DB_CreateTable(g_db, DB_SERVICE);
	DB_CreateTable(g_db, DB_NETWORK);
	DB_CreateTable(g_db, DB_BLACKLIST);
	wprintf_s(L"Database Initialized\n");

	// Json Thread
	hJsonThread[0] = (HANDLE)_beginthreadex(NULL, 0, &JsonMainThreadProc, (LPVOID)NULL, 0, NULL);
	hJsonThread[1] = (HANDLE)_beginthreadex(NULL, 0, &JsonTrapThreadProc, (LPVOID)NULL, 0, NULL);
	g_runJsonMainThread = TRUE;
	g_runJsonTrapThread = TRUE;
	wprintf_s(L"Json Threads running\n\n");

	BOOL first = TRUE;

	while (TRUE) {
		SnapCurrentStatus(
			sqlitePrc, 
			sqliteReg1, 
			sqliteReg2, 
			sqliteReg3, 
			sqliteReg4,
			sqliteSvc, 
			sqliteNet1,
			sqliteNet2,
			structSize,
			JSON_EnqTrapQueue,
			&trapQueue,
			g_db,
			2
		);

		if (initSizeCheck == 0) {
			free(sqlitePrc);
			free(sqliteReg1);
			free(sqliteReg2);
			free(sqliteReg3);
			free(sqliteReg4);
			free(sqliteSvc);
			free(sqliteNet1);
			free(sqliteNet2);
			initSizeCheck = 1;
		}

		sqlitePrc = (TWTL_DB_PROCESS*)calloc(structSize[0] + 1, sizeof(TWTL_DB_PROCESS));
		sqliteReg1 = (TWTL_DB_REGISTRY*)calloc(structSize[1] + 1, sizeof(TWTL_DB_REGISTRY));
		sqliteReg2 = (TWTL_DB_REGISTRY*)calloc(structSize[2] + 1, sizeof(TWTL_DB_REGISTRY));
		sqliteReg3 = (TWTL_DB_REGISTRY*)calloc(structSize[3] + 1, sizeof(TWTL_DB_REGISTRY));
		sqliteReg4 = (TWTL_DB_REGISTRY*)calloc(structSize[4] + 1, sizeof(TWTL_DB_REGISTRY));
		sqliteSvc = (TWTL_DB_SERVICE*)calloc(structSize[5] + 1, sizeof(TWTL_DB_SERVICE));
		sqliteNet1 = (TWTL_DB_NETWORK*)calloc(structSize[6] + 1, sizeof(TWTL_DB_NETWORK));
		sqliteNet2 = (TWTL_DB_NETWORK*)calloc(structSize[7] + 1, sizeof(TWTL_DB_NETWORK));

		SnapCurrentStatus(
			sqlitePrc,
			sqliteReg1,
			sqliteReg2,
			sqliteReg3,
			sqliteReg4,
			sqliteSvc,
			sqliteNet1,
			sqliteNet2,
			NULL,
			JSON_EnqTrapQueue,
			&trapQueue,
			g_db,
			0);
		lock = 0;

		if (first)
		{
			g_dbLock = TRUE;
			DB_Insert(g_db, DB_REG_HKCU_RUN, sqliteReg1, structSize[1]);
			DB_Insert(g_db, DB_REG_HKLM_RUN, sqliteReg2, structSize[2]);
			DB_Insert(g_db, DB_REG_HKCU_RUNONCE, sqliteReg3, structSize[3]);
			DB_Insert(g_db, DB_REG_HKLM_RUNONCE, sqliteReg4, structSize[4]);
			DB_Insert(g_db, DB_SERVICE, sqliteSvc, structSize[5]);
			g_dbLock = FALSE;
			first = FALSE;
		}

		DiffReg(sqliteReg1, sqliteReg2, sqliteReg3, sqliteReg4, sqliteSvc, structSize);

		lock = 1;
		free(sqlitePrc);
		free(sqliteReg1);
		free(sqliteReg2);
		free(sqliteReg3);
		free(sqliteReg4);
		free(sqliteSvc);
		free(sqliteNet1);
		free(sqliteNet2);

		if (_kbhit())
		{
			int input = _getch();
			if (input == 'q' || input == 'Q')
				break;
		}
		Sleep(5000);
	}
	
	wprintf_s(L"\nTerminating Json Threads...\n");
	g_runJsonMainThread = FALSE;
	g_runJsonTrapThread = FALSE;
	WaitForMultipleObjects(2, hJsonThread, TRUE, 5000);
	CloseHandle(hJsonThread[0]);
	CloseHandle(hJsonThread[1]);

	DB_Close(g_db);

	wprintf_s(L"\nBye!\n");
	
	return 0;
}

void DiffReg(TWTL_DB_REGISTRY* nowHkcuRun, TWTL_DB_REGISTRY* nowHklmRun, TWTL_DB_REGISTRY* nowHkcuRunOnce, TWTL_DB_REGISTRY* nowHklmRunOnce, TWTL_DB_SERVICE* nowServices, DWORD nowSize[])
{
	// Get database's row count
	int countLastHkcuRun = 0;
	int countLastHklmRun = 0;
	int countLastHkcuRunOnce = 0;
	int countLastHklmRunOnce = 0;
	int countLastServices = 0;

	TWTL_DB_REGISTRY* lastHkcuRun = NULL;
	TWTL_DB_REGISTRY* lastHklmRun = NULL;
	TWTL_DB_REGISTRY* lastHkcuRunOnce = NULL;
	TWTL_DB_REGISTRY* lastHklmRunOnce = NULL;
	TWTL_DB_SERVICE* lastServices = NULL;


	DB_Select(g_db, DB_REG_HKCU_RUN, NULL, &countLastHkcuRun, NULL);
	lastHkcuRun = (TWTL_DB_REGISTRY*)calloc(countLastHkcuRun + 1, sizeof(TWTL_DB_REGISTRY));
	DB_Select(g_db, DB_REG_HKCU_RUN, lastHkcuRun, &countLastHkcuRun, NULL);
	
	DB_Select(g_db, DB_REG_HKLM_RUN, NULL, &countLastHklmRun, NULL);
	lastHklmRun = (TWTL_DB_REGISTRY*)calloc(countLastHklmRun + 1, sizeof(TWTL_DB_REGISTRY));
	DB_Select(g_db, DB_REG_HKLM_RUN, lastHklmRun, &countLastHklmRun, NULL);
	
	DB_Select(g_db, DB_REG_HKCU_RUNONCE, NULL, &countLastHkcuRunOnce, NULL);
	lastHkcuRunOnce = (TWTL_DB_REGISTRY*)calloc(countLastHkcuRunOnce + 1, sizeof(TWTL_DB_REGISTRY));
	DB_Select(g_db, DB_REG_HKCU_RUNONCE, lastHkcuRunOnce, &countLastHkcuRunOnce, NULL);

	DB_Select(g_db, DB_REG_HKLM_RUNONCE, NULL, &countLastHklmRunOnce, NULL);
	lastHklmRunOnce = (TWTL_DB_REGISTRY*)calloc(countLastHklmRunOnce + 1, sizeof(TWTL_DB_REGISTRY));
	DB_Select(g_db, DB_REG_HKLM_RUNONCE, lastHklmRunOnce, &countLastHklmRunOnce, NULL);
	
	DB_Select(g_db, DB_SERVICE, NULL, &countLastServices, NULL);
	lastServices = (TWTL_DB_SERVICE*)calloc(countLastServices + 1, sizeof(TWTL_DB_SERVICE));
	DB_Select(g_db, DB_SERVICE, lastServices, &countLastServices, NULL);

	// Services
	// Find new entries
	BOOL sent = FALSE;
	for (DWORD idx_n = 0; idx_n < nowSize[5]; idx_n++)
	{
		BOOL found = FALSE;
		for (int idx_l = 0; idx_l < countLastServices; idx_l++)
		{
			if (StrCmpW(nowServices[idx_n].key, lastServices[idx_l].key) == 0)
			{
				found = TRUE;
				break;
			}
		}

		if (found == FALSE)
		{ // Newly created!
			sent = TRUE;
		}
	}

	if (sent == FALSE)
	{
		for (int i = 0; i < 4; i++)
		{
			int countLast = 0;
			DWORD countNow = 0;
			TWTL_DB_REGISTRY* dbLastReg = NULL;
			TWTL_DB_REGISTRY* dbNowReg = NULL;
			switch (i)
			{
			case 0:
				countLast = countLastHkcuRun;
				countNow = nowSize[1];
				dbLastReg = lastHkcuRun;
				dbNowReg = nowHkcuRun;
				break;
			case 2:
				countLast = countLastHkcuRunOnce;
				countNow = nowSize[3];
				dbLastReg = lastHkcuRunOnce;
				dbNowReg = nowHkcuRunOnce;
				break;
			case 1:
				countLast = countLastHklmRun;
				countNow = nowSize[2];
				dbLastReg = lastHklmRun;
				dbNowReg = nowHklmRun;
				break;
			case 3:
				countLast = countLastHklmRunOnce;
				countNow = nowSize[4];
				dbLastReg = lastHklmRunOnce;
				dbNowReg = nowHklmRunOnce;
				break;
			}

			// Find new entries
			for (DWORD idx_n = 0; idx_n < countNow; idx_n++)
			{
				BOOL found = FALSE;
				for (int idx_l = 0; idx_l < countLast; idx_l++)
				{
					if (StrCmpW(dbNowReg[idx_n].value, dbLastReg[idx_l].value) == 0)
					{
						found = TRUE;
						break;
					}
				}

				if (found == FALSE)
				{ // Newly created!
					sent = TRUE;
				}
			}

			if (sent)
				break;
		}
	}

	if (sent)
		JSON_EnqTrapQueue(&trapQueue, "/Reg/Short/");

	// Finalize
	free(lastHkcuRun);
	free(lastHklmRun);
	free(lastHkcuRunOnce);
	free(lastHklmRunOnce);
	free(lastServices);
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
