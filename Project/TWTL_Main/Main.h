#pragma once

#ifdef _DEBUG
#pragma comment(lib, "..\\Debug\\TWTL_Snapshot.lib")
#else
#pragma comment(lib, "..\\Release\\TWTL_Snapshot.lib")
#endif

#include "Hooking.h"

#define PRONAME_MAX	 260
#define REGNAME_MAX	 255
#define REGVALUE_MAX 16383

__declspec(dllimport)
BOOL
__stdcall
SnapCurrentStatus(
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

// Database
// Library headers
#include "..\TWTL_Database\sqlite3.h"

enum DB_TABLE_TYPE
{
	DB_PROCESS,
	DB_REG_HKLM_RUN, DB_REG_HKLM_RUNONCE, DB_REG_HKCU_RUN, DB_REG_HKCU_RUNONCE,
	DB_SERVICE,
	DB_NETWORK
};

#define DB_MAX_PROC_NAME 64
// -1 means NULL!
typedef struct twtl_db_process
{
	int64_t time; // Unix Epoch
	int32_t pid; // Process ID
	int32_t ppid; // Process Parent ID
	WCHAR process_name[DB_MAX_PROC_NAME]; // Ex) explorer.exe
	WCHAR process_path[MAX_PATH]; // Ex) C:\\Windows\\System32\\explorer.exe
} TWTL_DB_PROCESS;

#define DB_MAX_REG_PATH 255
#define DB_MAX_REG_VALUE 16384
#define DB_MAX_REG_DATA 2048

// https://msdn.microsoft.com/ko-kr/library/windows/desktop/ms724872(v=vs.85).aspx
typedef struct twtl_db_registry
{
	int64_t time; // Unix Epoch
	WCHAR path[DB_MAX_REG_PATH]; // Ex) HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run
	WCHAR value[DB_MAX_REG_VALUE]; // Ex) V3 Application
	int32_t type; // Ex) REG_SZ
	WCHAR data[DB_MAX_REG_DATA]; // Ex) "C:\\Program Files\\AhnLab\\V3Lite30\\V3Lite.exe" /tray
} TWTL_DB_REGISTRY;

typedef struct twtl_service
{
	int64_t time; // Unix Epoch
	WCHAR key[DB_MAX_REG_PATH]; // Ex) HomeGroup
} TWTL_DB_SERVICE;

typedef struct twtl_db_network
{
	int64_t time; // Unix Epoch
	int32_t ipv4; // IPv4
	int16_t port; // Port
} TWTL_DB_NETWORK;

#define MAX_SQL_BUF 256


sqlite3* __stdcall DB_Connect(LPCWSTR dbFilePath);
BOOL __stdcall DB_Close(sqlite3 *db);
BOOL __stdcall DB_CreateTable(sqlite3 *db, DB_TABLE_TYPE type);
BOOL __stdcall DB_Insert(sqlite3 *db, DB_TABLE_TYPE type, void* data);
BOOL __stdcall DB_Select(sqlite3 *db, DB_TABLE_TYPE type, void* data, WCHAR* sql_where);

