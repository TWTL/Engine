#pragma once

#include "stdafx.h"

enum DB_TABLE_TYPE 
{ 
	DB_PROCESS,
	DB_REG_HKLM_RUN, DB_REG_HKLM_RUNONCE, DB_REG_HKCU_RUN, DB_REG_HKCU_RUNONCE,
	DB_SERVICE,
	DB_NETWORK,
	DB_BLACKLIST
};

#define DB_MAX_PROC_NAME 64
// -1 means NULL!
typedef struct twtl_db_process
{
	int32_t pid; // Process ID
	int32_t ppid; // Process Parent ID
	WCHAR process_name[DB_MAX_PROC_NAME]; // Ex) explorer.exe
	WCHAR process_path[MAX_PATH]; // Ex) C:\\Windows\\System32\\explorer.exe
} TWTL_DB_PROCESS;

#define DB_MAX_REG_PATH 256
#define DB_MAX_REG_VALUE 16384
#define DB_MAX_REG_NAME 2048

// https://msdn.microsoft.com/ko-kr/library/windows/desktop/ms724872(v=vs.85).aspx
typedef struct twtl_db_registry
{
	WCHAR path[DB_MAX_REG_PATH]; // Ex) HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run
	WCHAR value[DB_MAX_REG_VALUE]; // Ex) "C:\\Program Files\\AhnLab\\V3Lite30\\V3Lite.exe" /tray
	int32_t type; // Ex) REG_SZ
	WCHAR name[DB_MAX_REG_NAME]; // Ex) V3 Application
} TWTL_DB_REGISTRY;

typedef struct twtl_service
{
	WCHAR key[DB_MAX_REG_PATH]; // Ex) HomeGroup
	WCHAR image_path[DB_MAX_REG_PATH]; 
} TWTL_DB_SERVICE;

// Use 0 to show this value is invalid
typedef struct twtl_db_network
{
	uint32_t src_ipv4; // Src IPv4
	uint32_t dest_ipv4; // Dest IPv4
	uint16_t src_port; // Src Port
	uint16_t dest_port; // Dest Port
	uint16_t pid; // PID
	uint16_t is_dangerous; // Boolean
} TWTL_DB_NETWORK;

#define DB_MAX_FILE_PATH 260
typedef struct twtl_db_blacklist
{
	WCHAR image_path[DB_MAX_FILE_PATH];
} TWTL_DB_BLACKLIST;

#define MAX_SQL_BUF 1024

TWTL_DATABASE_API sqlite3* __stdcall DB_Connect(LPCWSTR dbFilePath);
TWTL_DATABASE_API BOOL __stdcall DB_Close(sqlite3 *db);
TWTL_DATABASE_API BOOL __stdcall DB_CreateTable(sqlite3 *db, DB_TABLE_TYPE type);
TWTL_DATABASE_API BOOL __stdcall DB_Insert(sqlite3 *db, DB_TABLE_TYPE type, void* data, int count);
TWTL_DATABASE_API BOOL __stdcall DB_Select(sqlite3 *db, DB_TABLE_TYPE type, void* data, int* count, WCHAR* sql_where);
TWTL_DATABASE_API BOOL __stdcall DB_Delete(sqlite3 *db, DB_TABLE_TYPE type, WCHAR* sql_where);

