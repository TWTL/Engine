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

TWTL_DATABASE_API BOOL __stdcall DB_CreateOrOpenTable(sqlite3 *db, LPCWSTR table)
{
	// Create table 
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
		fprintf(stderr, "Failed to fetch data: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return FALSE;
	}

	// Run SQL
	ret = sqlite3_step(stmt);
	if (ret == SQLITE_ROW) {
		printf("%s\n", sqlite3_column_text(stmt, 0));
		return FALSE;
	}

	sqlite3_finalize(stmt);
	return TRUE;
}