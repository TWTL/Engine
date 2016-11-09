#pragma once

#include "stdafx.h"

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

TWTL_DATABASE_API sqlite3* __stdcall DB_Connect(LPCWSTR dbFilePath);
TWTL_DATABASE_API BOOL __stdcall DB_Close(sqlite3 *db);
TWTL_DATABASE_API BOOL __stdcall DB_CreateTable(sqlite3 *db, DB_TABLE_TYPE type);
TWTL_DATABASE_API BOOL __stdcall DB_Insert(sqlite3 *db, DB_TABLE_TYPE type, void* data);
TWTL_DATABASE_API BOOL __stdcall DB_Select(sqlite3 *db, DB_TABLE_TYPE type, void* data, WCHAR* sql_where);
BOOL __stdcall DB_RunSQL(sqlite3 *db, LPCWSTR sql);
