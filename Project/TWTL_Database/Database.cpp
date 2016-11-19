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
( in )	sqlite3* db :
  SQLite3 context

Return value :
  FALSE (0) : Failure
  TRUE  (1) : Sucess
*/
TWTL_DATABASE_API BOOL __stdcall DB_Close(sqlite3 *db) {
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

/*
Description : Create table if not exists

Parameters :
( in )	sqlite3* db :
  SQLite3 context
( in )  DB_TABLE_TYPE type :
  Create which database table?

Return value :
  FALSE (0) : Failure
  TRUE  (1) : Sucess
*/
TWTL_DATABASE_API BOOL __stdcall DB_CreateTable(sqlite3 *db, DB_TABLE_TYPE type) {
	WCHAR* sql = NULL;
	// Main Port 5259
	// Create table
	switch (type) {
	case DB_PROCESS:
		sql = L"CREATE TABLE IF NOT EXISTS snapshot_process("
			L"idx INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL"
			L"time INTEGER NOT NULL"
			L"pid INTEGER"
			L"ppid INTEGER"
			L"process_name TEXT NOT NULL"
			L"process_path TEXT NOT NULL);";
		break;
	case DB_REG_HKLM_RUN:
		sql = L"CREATE TABLE IF NOT EXISTS snapshot_reg_hklm_run("
			L"idx INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL"
			L"time INTEGER NOT NULL"
			L"value TEXT NOT NULL"
			L"type INTEGER NOT NULL"
			L"data TEXT NOT NULL);";
		break;
	case DB_REG_HKLM_RUNONCE:
		sql = L"CREATE TABLE IF NOT EXISTS snapshot_reg_hklm_runonce("
			L"idx INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL"
			L"time INTEGER NOT NULL"
			L"value TEXT NOT NULL"
			L"type INTEGER NOT NULL"
			L"data TEXT NOT NULL);";
		break;
	case DB_REG_HKCU_RUN:
		sql = L"CREATE TABLE IF NOT EXISTS snapshot_reg_hkcu_run("
			L"idx INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL"
			L"time INTEGER NOT NULL"
			L"value TEXT NOT NULL"
			L"type INTEGER NOT NULL"
			L"data TEXT NOT NULL);";
		break;
	case DB_REG_HKCU_RUNONCE:
		sql = L"CREATE TABLE IF NOT EXISTS snapshot_reg_hkcu_runonce("
			L"idx INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL"
			L"time INTEGER NOT NULL"
			L"value TEXT NOT NULL"
			L"type INTEGER NOT NULL"
			L"data TEXT NOT NULL);";
		break;
	case DB_SERVICE:
		sql = L"CREATE TABLE IF NOT EXISTS snapshot_service("
			L"idx INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL"
			L"time INTEGER NOT NULL"
			L"key TEXT NOT NULL);";
		break;
	case DB_NETWORK:
		sql = L"CREATE TABLE IF NOT EXISTS snapshot_network("
			L"idx INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL"
			L"time TEXT NOT NULL"
			L"ip TEXT NOT NULL"
			L"port INTEGER);";
		break;
	default:
#ifdef _DEBUG
		fprintf(stderr, "[DB_CreateTable] Invalid type %d\n", type);
#endif
		return FALSE;
		break;
	}

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
		return FALSE;
	}
#endif

	// Finalize Query
	ret = sqlite3_finalize(stmt);
	if (ret != SQLITE_OK) {
#ifdef _DEBUG
		fprintf(stderr, "[DB_Insert] Failed to fetch data: %s\n", sqlite3_errmsg(db));
#endif
		return FALSE;
	}

	return TRUE;
}

/*
Description : Insert data into DB

Parameters :
( in )	sqlite3 *db :
  Pointer to sqlite3 context
( in )  DB_TABLE_TYPE type:
  data's type
( in )  viod* data:
  Pointer to data structure, casted as void*

Return value :
  FALSE (0) : Failure
  TRUE  (1) : Sucess
*/

TWTL_DATABASE_API BOOL __stdcall DB_Insert(sqlite3 *db, DB_TABLE_TYPE type, void* data)
{
	int ret = 0;
	WCHAR sql[MAX_SQL_BUF] = { 0 };
	TWTL_DB_PROCESS* proc = NULL;
	TWTL_DB_REGISTRY* reg = NULL;
	TWTL_DB_SERVICE* srv = NULL;
	TWTL_DB_NETWORK* net = NULL;

	switch (type) {
	case DB_PROCESS:
		proc = (TWTL_DB_PROCESS*)data;
		if (proc->pid && proc->ppid) // PID and PPID is valid
			StringCchPrintfW(sql, MAX_SQL_BUF, L"INSERT INTO snapshot_process(time, pid, ppid, process_name, process_path) VALUES(%lld, %ld, %ld, '%s', '%s'); ", proc->time, proc->pid, proc->ppid, proc->process_name, proc->process_path);
		else // No PID and PPID
			StringCchPrintfW(sql, MAX_SQL_BUF, L"INSERT INTO snapshot_process(time, process_name, process_path) VALUES(%lld, '%s', '%s'); ", proc->time, proc->process_name, proc->process_path);
		break;
	case DB_REG_HKLM_RUN:
		reg = (TWTL_DB_REGISTRY*)data;
		StringCchPrintfW(sql, MAX_SQL_BUF, L"INSERT INTO snapshot_reg_hklm_run(time, path, value, type, data) VALUES(%lld, '%s', '%s', %ld, '%s'); ", reg->time, reg->path, reg->value, reg->type, reg->data);
		break;
	case DB_REG_HKLM_RUNONCE:
		reg = (TWTL_DB_REGISTRY*)data;
		StringCchPrintfW(sql, MAX_SQL_BUF, L"INSERT INTO snapshot_reg_hklm_runonce(time, path, value, type, data) VALUES(%lld, '%s', '%s', %ld, '%s'); ", reg->time, reg->path, reg->value, reg->type, reg->data);
		break;
	case DB_REG_HKCU_RUN:
		reg = (TWTL_DB_REGISTRY*)data;
		StringCchPrintfW(sql, MAX_SQL_BUF, L"INSERT INTO snapshot_reg_hkcu_run(time, path, value, type, data) VALUES(%lld, '%s', '%s', %ld, '%s'); ", reg->time, reg->path, reg->value, reg->type, reg->data);
		break;
	case DB_REG_HKCU_RUNONCE:
		reg = (TWTL_DB_REGISTRY*)data;
		StringCchPrintfW(sql, MAX_SQL_BUF, L"INSERT INTO snapshot_reg_hkcu_runonce(time, path, value, type, data) VALUES(%lld, '%s', '%s', %ld, '%s'); ", reg->time, reg->path, reg->value, reg->type, reg->data);
		break;
	case DB_SERVICE:
		srv = (TWTL_DB_SERVICE*)data;
		StringCchPrintfW(sql, MAX_SQL_BUF, L"INSERT INTO snapshot_service(time, key) VALUES(%lld, '%s'); ", srv->time, srv->key);
		break;
	case DB_NETWORK:
		net = (TWTL_DB_NETWORK*)data;
		StringCchPrintfW(sql, MAX_SQL_BUF, L"INSERT INTO snapshot_network VALUES(%lld, %ld, %ld); ", net->time, net->ipv4, net->port);
		break;
	default:
		return FALSE;
	}

	sqlite3_stmt *stmt;

	// Prepare statement
	ret = sqlite3_prepare16_v2(db, sql, -1, &stmt, NULL);
	if (ret != SQLITE_OK) {
#ifdef _DEBUG
		fprintf(stderr, "[DB_Insert] Failed to fetch data: %s\n", sqlite3_errmsg(db));
#endif
		return FALSE;
	}

	// Run SQL
	ret = sqlite3_step(stmt);
#ifdef _DEBUG
	if (ret != SQLITE_ROW)
		return FALSE;
#endif

	return TRUE;

	// Finalize Query
	ret = sqlite3_finalize(stmt);
	if (ret != SQLITE_OK) {
#ifdef _DEBUG
		fprintf(stderr, "[DB_Insert] Failed to fetch data: %s\n", sqlite3_errmsg(db));
#endif
		return FALSE;
	}
}

/*
Description : Obtain data from DB
Parameters :
( in )	sqlite3 *db :
  Pointer to sqlite3 context
( in )  DB_TABLE_TYPE type:
  data's type
( out )  viod* data:
  Pointer to data structure, casted as void*
( in )  WCHAR* sql_where:
  SQL where statement
Return value :
  FALSE (0) : Failure
  TRUE  (1) : Sucess
*/
TWTL_DATABASE_API BOOL __stdcall DB_Select(sqlite3 *db, DB_TABLE_TYPE type, void* data, WCHAR* sql_where)
{
	int ret = 0;
	WCHAR sql[MAX_SQL_BUF] = { 0 };
	TWTL_DB_PROCESS* proc = NULL;
	TWTL_DB_REGISTRY* reg = NULL;
	TWTL_DB_SERVICE* srv = NULL;
	TWTL_DB_NETWORK* net = NULL;

	switch (type) {
	case DB_PROCESS:
		if (sql_where)
			StringCchPrintfW(sql, MAX_SQL_BUF, L"SELECT * from snapshot_process WHERE %s;", sql_where);
		else
			StringCchCopyW(sql, MAX_SQL_BUF, L"SELECT * from snapshot_process;");
		break;
	case DB_REG_HKLM_RUN:
		if (sql_where)
			StringCchPrintfW(sql, MAX_SQL_BUF, L"SELECT * from snapshot_reg_khlm_run WHERE %s;", sql_where);
		else
			StringCchCopyW(sql, MAX_SQL_BUF, L"SELECT * from snapshot_reg_khlm_runonce;");
		break;
	case DB_REG_HKLM_RUNONCE:
		if (sql_where)
			StringCchPrintfW(sql, MAX_SQL_BUF, L"SELECT * from snapshot_reg_khlm_runonce WHERE %s;", sql_where);
		else
			StringCchCopyW(sql, MAX_SQL_BUF, L"SELECT * from snapshot_reg_khlm_runonce;");
		break;
	case DB_REG_HKCU_RUN:
		if (sql_where)
			StringCchPrintfW(sql, MAX_SQL_BUF, L"SELECT * from snapshot_reg_khcu_run WHERE %s;", sql_where);
		else
			StringCchCopyW(sql, MAX_SQL_BUF, L"SELECT * from snapshot_reg_khcu_run;");
		break;
	case DB_REG_HKCU_RUNONCE:
		if (sql_where)
			StringCchPrintfW(sql, MAX_SQL_BUF, L"SELECT * from snapshot_reg_khcu_runonce WHERE %s;", sql_where);
		else
			StringCchCopyW(sql, MAX_SQL_BUF, L"SELECT * from snapshot_reg_khcu_runonce;");
		break;
	case DB_SERVICE:
		if (sql_where)
			StringCchPrintfW(sql, MAX_SQL_BUF, L"SELECT * from snapshot_service WHERE %s;", sql_where);
		else
			StringCchCopyW(sql, MAX_SQL_BUF, L"SELECT * from snapshot_service;");
		break;
	case DB_NETWORK:
		if (sql_where)
			StringCchPrintfW(sql, MAX_SQL_BUF, L"SELECT * from snapshot_network WHERE %s;", sql_where);
		else
			StringCchCopyW(sql, MAX_SQL_BUF, L"SELECT * from snapshot_network;");
		break;
	default:
		return FALSE;
	}

	sqlite3_stmt *stmt;

	// Prepare statement
	ret = sqlite3_prepare16_v2(db, sql, -1, &stmt, NULL);
	if (ret != SQLITE_OK) {
#ifdef _DEBUG
		fprintf(stderr, "[DB_Insert] Failed to prepare statement: %s\n", sqlite3_errmsg(db));
#endif
		return FALSE;
	}

	// Run SQL
	ret = sqlite3_step(stmt);
#ifdef _DEBUG
	if (ret != SQLITE_ROW)
		return FALSE;
#endif

	switch (type) {
	case DB_PROCESS:
		proc = (TWTL_DB_PROCESS*)data;
		// sqlite3_column_int64(stmt, 0); is idx, nothing meaningless
		proc->time = sqlite3_column_int64(stmt, 1);
		proc->pid = sqlite3_column_int(stmt, 2);
		proc->ppid = sqlite3_column_int(stmt, 3);
		StringCchCopyW(proc->process_name, DB_MAX_PROC_NAME, (const WCHAR*)sqlite3_column_text16(stmt, 4));
		StringCchCopyW(proc->process_path, MAX_PATH, (const WCHAR*)sqlite3_column_text16(stmt, 5));
		break;
	case DB_REG_HKLM_RUN:
	case DB_REG_HKLM_RUNONCE:
	case DB_REG_HKCU_RUN:
	case DB_REG_HKCU_RUNONCE:
		reg = (TWTL_DB_REGISTRY*)data;
		// sqlite3_column_int64(stmt, 0); is idx, nothing meaningless
		reg->time = sqlite3_column_int64(stmt, 1);
		StringCchCopyW(reg->path, DB_MAX_REG_PATH, (const WCHAR*)sqlite3_column_text16(stmt, 2));
		StringCchCopyW(reg->value, DB_MAX_REG_VALUE, (const WCHAR*)sqlite3_column_text16(stmt, 3));
		reg->type = sqlite3_column_int(stmt, 4);
		StringCchCopyW(reg->data, DB_MAX_REG_DATA, (const WCHAR*)sqlite3_column_text16(stmt, 5));
		break;
	case DB_SERVICE:
		srv = (TWTL_DB_SERVICE*)data;
		// sqlite3_column_int64(stmt, 0); is idx, nothing meaningless
		srv->time = sqlite3_column_int64(stmt, 1);
		StringCchCopyW(srv->key, DB_MAX_REG_PATH, (const WCHAR*)sqlite3_column_text16(stmt, 2));
		break;
	case DB_NETWORK:
		net = (TWTL_DB_NETWORK*)data;
		// sqlite3_column_int64(stmt, 0); is idx, nothing meaningless
		net->time = sqlite3_column_int64(stmt, 1);
		net->ipv4 = sqlite3_column_int(stmt, 2);
		net->port = (uint16_t)sqlite3_column_int(stmt, 3);
		break;
	default:
		return FALSE;
	}

	ret = sqlite3_finalize(stmt);
	if (ret != SQLITE_OK) {
#ifdef _DEBUG
		fprintf(stderr, "[DB_Insert] Failed to fetch data: %s\n", sqlite3_errmsg(db));
#endif
		return FALSE;
	}

	return TRUE;
}