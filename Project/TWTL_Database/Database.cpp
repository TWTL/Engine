#include "stdafx.h"
#include "Database.h"

// Go to https://www.sqlite.org/capi3ref.html for detailed sqlite3 APIs!

/*
Description : Open SQLite3 database

Parameters :
( in )	LPCWSTR dbFilePath :
  Path of .db file to open, in UTF-16
Return value :
  Pointer to sqlite3 context (sqlite3*)
*/
TWTL_DATABASE_API sqlite3* __stdcall DB_Connect(LPCWSTR dbFilePath) {
	sqlite3 *db;
	int ret = sqlite3_open16(dbFilePath, &db);
	if (ret) {
#ifdef _DEBUG
		fwprintf(stderr, L"[DB_Connect] Database cannot be opened : %s\n", (LPCWSTR) sqlite3_errmsg16(db));
#endif
		return NULL;
	}
	else {
#ifdef _DEBUG
		fwprintf(stderr, L"[DB_Connect] Database opened successfully\n");
#endif
		return db;
	}
}

/*
Description : Close sqlite3 context

Parameters :
( in )	char dbFilePath :
Path of .db file to open
Return value :
Pointer to sqlite3 context (sqlite3*)
*/
TWTL_DATABASE_API BOOL __stdcall DB_Close(sqlite3 *db)
{
	if (db == NULL) {
#ifdef _DEBUG
		fwprintf(stderr, L"[DB_Close] Cannot close NULL pointer\n");
#endif
		return FALSE;
	}

	int ret = sqlite3_close(db);
	if (ret == SQLITE_OK) { // Success
#ifdef _DEBUG
		fwprintf(stderr, L"[DB_Close] Database successfully closed\n");
#endif
		return TRUE;
	}
	else { // Failure, maybe SQLITE_BUSY value?
#ifdef _DEBUG
		fwprintf(stderr, L"[DB_Close] Database cannot be closed : SQLite 3 error code %d\n", ret);
#endif
		return FALSE;
	}
}

TWTL_DATABASE_API BOOL __stdcall DB_CreateTable(sqlite3 *db, DB_TABLE_TYPE type)
{
	WCHAR* sql = NULL;

	// Create table
	switch (type)
	{
	case DB_PROCESS:
		sql = L"CREATE TABLE IF NOT EXISTS snapshot_process("
			L"idx INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL"
			L"time TEXT NOT NULL"
			L"process_name TEXT NOT NULL"
			L"process_path TEXT NOT NULL);";
		break;
	case DB_HKLM_RUN:
		sql = L"CREATE TABLE IF NOT EXISTS snapshot_hklm_run("
			L"idx INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL"
			L"time TEXT NOT NULL"
			L"value TEXT NOT NULL"
			L"type INTEGER NOT NULL"
			L"data TEXT NOT NULL);";
		break;
	case DB_HKLM_RUNONCE:
		sql = L"CREATE TABLE IF NOT EXISTS snapshot_hklm_runonce("
			L"idx INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL"
			L"time TEXT NOT NULL"
			L"value TEXT NOT NULL"
			L"type INTEGER NOT NULL"
			L"data TEXT NOT NULL);";
		break;
	case DB_HKCU_RUN:
		sql = L"CREATE TABLE IF NOT EXISTS snapshot_hkcu_run("
			L"idx INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL"
			L"time TEXT NOT NULL"
			L"value TEXT NOT NULL"
			L"type INTEGER NOT NULL"
			L"data TEXT NOT NULL);";
		break;
	case DB_HKCU_RUNONCE:
		sql = L"CREATE TABLE IF NOT EXISTS snapshot_hkcu_runonce("
			L"idx INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL"
			L"time TEXT NOT NULL"
			L"value TEXT NOT NULL"
			L"type INTEGER NOT NULL"
			L"data TEXT NOT NULL);";
		break;
	case DB_NETWORK:
		sql = L"CREATE TABLE IF NOT EXISTS snapshot_network("
			L"idx INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL"
			L"time TEXT NOT NULL"
			L"ip TEXT NOT NULL"
			L"port INTEGER NOT NULL);";
		break;
	default:
#ifdef _DEBUG
		fprintf(stderr, "[DB_CreateTable] Invalid type %d\n", type);
#endif
		return FALSE;
		break;
	}

	if (!DB_RunSQL(db, sql))
	{ // Failure
#ifdef _DEBUG
		fprintf(stderr, "[DB_CreateTable] DB_RunSQL failure\n");
#endif
		return FALSE;
	}

	return TRUE;
}

/*
Description : Internal function to run one sql statement

Parameters :
( in )	sqlite3 *db :
  Pointer to sqlite3 context
Return value :
  FALSE (0) : Failure
  TRUE  (1) : Sucess
*/
BOOL __stdcall DB_RunSQL(sqlite3 *db, LPCWSTR sql)
{
	sqlite3_stmt *stmt;
	int ret = 0;
	
	// Prepare statement
	ret = sqlite3_prepare16_v2(db, sql, -1, &stmt, NULL);
	if (ret != SQLITE_OK) {
#ifdef _DEBUG
		fprintf(stderr, "[DB_RunSQL] Failed to fetch data: %s\n", sqlite3_errmsg(db));
#endif
		sqlite3_close(db);
		return FALSE;
	}

	// Run SQL
	ret = sqlite3_step(stmt);
#ifdef _DEBUG
	if (ret == SQLITE_ROW) {
		printf("%s\n", sqlite3_column_text(stmt, 0));
		return FALSE;
	}
#endif

	sqlite3_finalize(stmt);
	return TRUE;
}