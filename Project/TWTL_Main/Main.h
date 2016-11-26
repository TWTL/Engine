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

#define PRONAME_MAX	 260
#define REGNAME_MAX	 255
#define REGVALUE_MAX 16383
// Database
// Library headers

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
#define DB_MAX_REG_NAME 2048

// https://msdn.microsoft.com/ko-kr/library/windows/desktop/ms724872(v=vs.85).aspx
typedef struct twtl_db_registry
{
	int64_t time; // Unix Epoch
	WCHAR path[DB_MAX_REG_PATH]; // Ex) HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run
	WCHAR value[DB_MAX_REG_VALUE]; // Ex) V3 Application
	int32_t type; // Ex) REG_SZ
	WCHAR name[DB_MAX_REG_NAME]; // Ex) "C:\\Program Files\\AhnLab\\V3Lite30\\V3Lite.exe" /tray
} TWTL_DB_REGISTRY;

typedef struct twtl_service
{
	int64_t time; // Unix Epoch
	WCHAR key[DB_MAX_REG_PATH]; // Ex) HomeGroup
	WCHAR image_path[DB_MAX_REG_PATH];
} TWTL_DB_SERVICE;


// Use 0 to show this value is invalid
typedef struct twtl_db_network
{
	int64_t time; // Unix Epoch
	uint32_t src_ipv4; // Src IPv4
	uint32_t dest_ipv4; // Dest IPv4
	uint16_t src_port; // Src Port
	uint16_t dest_port; // Dest Port
	uint16_t pid; // PID
} TWTL_DB_NETWORK;

#define MAX_SQL_BUF 256

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

