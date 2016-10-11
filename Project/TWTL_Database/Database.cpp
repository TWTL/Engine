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
TWTL_DATABASE_API sqlite3* __stdcall TWTL_DB_Connect(LPCWSTR dbFilePath) {
	sqlite3 *db;
	int ret = sqlite3_open16(dbFilePath, &db);
	if (ret) {
#ifdef _DEBUG
		fwprintf(stderr, L"[TWTL_DB_Connect] Database cannot be opened : %s\n", (LPCWSTR) sqlite3_errmsg16(db));
#endif
		return NULL;
	}
	else {
#ifdef _DEBUG
		fwprintf(stderr, L"[TWTL_DB_Connect] Database opened successfully\n");
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
TWTL_DATABASE_API BOOL __stdcall TWTL_DB_Close(sqlite3 *db)
{
	if (db == NULL) {
#ifdef _DEBUG
		fwprintf(stderr, L"[TWTL_DB_Close] Cannot close NULL pointer\n");
#endif
		return FALSE;
	}

	int ret = sqlite3_close(db);
	if (ret == SQLITE_OK) { // Success
#ifdef _DEBUG
		fwprintf(stderr, L"[TWTL_DB_Close] Database successfully closed\n");
#endif
		return TRUE;
	}
	else { // Failure, maybe SQLITE_BUSY value?
#ifdef _DEBUG
		fwprintf(stderr, L"[TWTL_DB_Close] Database cannot be closed : SQLite 3 error code %d\n", ret);
#endif
		return FALSE;
	}
}