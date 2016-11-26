#pragma once

#ifdef _DEBUG
#pragma comment(lib, "..\\Debug\\TWTL_Snapshot.lib")
#pragma comment(lib, "..\\Debug\\TWTL_Database.lib")
#else
#pragma comment(lib, "..\\Release\\TWTL_Snapshot.lib")
#pragma comment(lib, "..\\Release\\TWTL_Database.lib")
#endif

#include "Hooking.h"
#include "..\TWTL_Database\sqlite3.h"
#include "..\TWTL_Database\Database.h"

#define PRONAME_MAX	 260
#define REGNAME_MAX	 255
#define REGVALUE_MAX 16383

__declspec(dllimport)
BOOL
__stdcall
SnapCurrentStatus(
	TWTL_DB_PROCESS*  sqlitePrc,
	TWTL_DB_REGISTRY* sqliteReg1,
	TWTL_DB_REGISTRY* sqliteReg2,
	TWTL_DB_REGISTRY* sqliteReg3,
	TWTL_DB_REGISTRY* sqliteReg4,
	TWTL_DB_SERVICE*  sqliteSvc,
	TWTL_DB_NETWORK*  sqliteNet1,
	TWTL_DB_NETWORK*  sqliteNet2,
	DWORD structSize[],
	CONST DWORD32 mode
);

__declspec(dllimport)
BOOL
__stdcall
TerminateCurrentProcess(
	CONST DWORD32 targetPID,
	CONST DWORD mode
);

__declspec(dllimport)
BOOL
__stdcall
DeleteRunKey(
	TCHAR CONST keyName[REGNAME_MAX],
	CONST DWORD32 targetKey
);

__declspec(dllimport)
BOOL
__stdcall
SetPrivilege(
	LPCTSTR lpszPrivilege,
	BOOL bEnablePrivilege
);

__declspec(dllimport)
VOID
__stdcall
DelayWait(
	CONST DWORD dwMillisecond
);

__declspec(dllimport)
VOID
__stdcall
ErrMsg(
);

void BinaryDump(const uint8_t buf[], const uint32_t bufsize);

sqlite3* __stdcall DB_Connect(LPCWSTR dbFilePath);
BOOL __stdcall DB_Close(sqlite3 *db);
BOOL __stdcall DB_CreateTable(sqlite3 *db, DB_TABLE_TYPE type);
BOOL __stdcall DB_Insert(sqlite3 *db, DB_TABLE_TYPE type, void* data);
BOOL __stdcall DB_Select(sqlite3 *db, DB_TABLE_TYPE type, void* data, WCHAR* sql_where);

