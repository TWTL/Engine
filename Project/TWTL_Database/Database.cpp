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
	int ret;
	// ret = sqlite3_config(SQLITE_CONFIG_SERIALIZED);
	ret = sqlite3_config(SQLITE_CONFIG_MULTITHREAD);
	if (ret) {
#ifdef _DEBUG
		fwprintf(stderr, L"[DB_Connect] Multithread failed\n");
#endif
		return NULL;
	}
	else {
#ifdef _DEBUG
		fwprintf(stderr, L"[DB_Connect] Multithread success\n");
#endif
	}

	ret = sqlite3_open16(dbFilePath, &db);
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
		sql = L"CREATE TABLE IF NOT EXISTS snapshot_process( "
			L"idx INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
			L"time INTEGER, "
			L"pid INTEGER, "
			L"ppid INTEGER, "
			L"process_name TEXT NOT NULL, "
			L"process_path TEXT NOT NULL);";
		break;
	case DB_REG_HKLM_RUN:
		sql = L"CREATE TABLE IF NOT EXISTS snapshot_reg_hklm_run( "
			L"idx INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
			L"time INTEGER, "
			L"path TEXT NOT NULL, "
			L"value TEXT NOT NULL, "
			L"type INTEGER NOT NULL, "
			L"name TEXT NOT NULL);";
		break;
	case DB_REG_HKLM_RUNONCE:
		sql = L"CREATE TABLE IF NOT EXISTS snapshot_reg_hklm_runonce( "
			L"idx INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
			L"time INTEGER, "
			L"path TEXT NOT NULL, "
			L"value TEXT NOT NULL, "
			L"type INTEGER NOT NULL, "
			L"name TEXT NOT NULL);";
		break;
	case DB_REG_HKCU_RUN:
		sql = L"CREATE TABLE IF NOT EXISTS snapshot_reg_hkcu_run( "
			L"idx INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
			L"time INTEGER, "
			L"path TEXT NOT NULL, "
			L"value TEXT NOT NULL, "
			L"type INTEGER NOT NULL, "
			L"name TEXT NOT NULL);";
		break;
	case DB_REG_HKCU_RUNONCE:
		sql = L"CREATE TABLE IF NOT EXISTS snapshot_reg_hkcu_runonce("
			L"idx INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
			L"time INTEGER, "
			L"path TEXT NOT NULL, "
			L"value TEXT NOT NULL, "
			L"type INTEGER NOT NULL, "
			L"name TEXT NOT NULL);";
		break;
	case DB_SERVICE:
		sql = L"CREATE TABLE IF NOT EXISTS snapshot_service("
			L"idx INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
			L"time INTEGER, "
			L"key TEXT NOT NULL, "
			L"image_path TEXT NOT NULL);";
		break;
	case DB_NETWORK:
		sql = L"CREATE TABLE IF NOT EXISTS snapshot_network("
			L"idx INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
			L"time INTEGER, "
			L"src_ipv4 INTEGER, "
			L"dest_ipv4 INTEGER, "
			L"src_port INTEGER, "
			L"dest_port INTEGER, "
			L"pid INTEGER NOT NULL, "
			L"is_dangerous INTEGER NOT NULL);";
		break;
	case DB_BLACKLIST:
		sql = L"CREATE TABLE IF NOT EXISTS snapshot_blacklist("
			L"idx INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
			L"time INTEGER, "
			L"image_path TEXT NOT NULL);";
		break;
	default:
#ifdef _DEBUG
		fprintf(stderr, "[DB_CreateTable] Invalid type %d\n", type);
#endif
		return FALSE;
		break;
	}

	sqlite3_stmt *stmt = NULL;
	int ret = 0;

	// Prepare statement
	ret = sqlite3_prepare16_v2(db, sql, -1, &stmt, NULL);
	if (ret != SQLITE_OK) {
#ifdef _DEBUG
		fprintf(stderr, "[DB_CreateTable] sqlite3_prepare16_v2() failed: %s\n", sqlite3_errmsg(db));
#endif
		return FALSE;
	}

	// Run SQL
	ret = sqlite3_step(stmt);
#ifdef _DEBUG
	if (ret == SQLITE_ROW) {
		return FALSE;
	}
#endif

	ret = sqlite3_reset(stmt);
	if (ret != SQLITE_OK)
	{
#ifdef _DEBUG
		fprintf(stderr, "[DB_CreateTable] sqlite3_reset() failed: %s\n", sqlite3_errmsg(db));
#endif
		return FALSE;
	}

	// Finalize Query
	ret = sqlite3_finalize(stmt);
	if (ret != SQLITE_OK) {
#ifdef _DEBUG
		fprintf(stderr, "[DB_CreateTable] sqlite3_finalize() failed: %s\n", sqlite3_errmsg(db));
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
  Pointer to data structure array, casted as void*
  Ex) TWTL_DB_NETWORK[255];
( in )  int count :
  Data array size
  Ex) 255

Return value :
  FALSE (0) : Failure
  TRUE  (1) : Sucess
*/
TWTL_DATABASE_API BOOL __stdcall DB_Insert(sqlite3 *db, DB_TABLE_TYPE type, void* data, int count)
{
	int ret = 0;

	if (count <= 0)
	{
		return FALSE;
	}

	for (int i = 0; i < count; i++)
	{
		WCHAR sql[MAX_SQL_BUF] = { 0 };
		TWTL_DB_PROCESS* proc = NULL;
		TWTL_DB_REGISTRY* reg = NULL;
		TWTL_DB_SERVICE* srv = NULL;
		TWTL_DB_NETWORK* net = NULL;
		TWTL_DB_BLACKLIST* black = NULL;

		switch (type) {
		case DB_PROCESS:
			proc = (TWTL_DB_PROCESS*)data;
			if (proc->pid && proc->ppid) // PID and PPID is valid
				StringCchPrintfW(sql, MAX_SQL_BUF, L"INSERT OR REPLACE INTO snapshot_process(pid, ppid, process_name, process_path) VALUES(%ld, %ld, '%s', '%s'); ", proc[i].pid, proc[i].ppid, proc[i].process_name, proc[i].process_path);
			else // No PID and PPID
				StringCchPrintfW(sql, MAX_SQL_BUF, L"INSERT OR REPLACE INTO snapshot_process(process_name, process_path) VALUES('%s', '%s'); ", proc[i].process_name, proc[i].process_path);
			break;
		case DB_REG_HKLM_RUN:
			reg = (TWTL_DB_REGISTRY*)data;
			StringCchPrintfW(sql, MAX_SQL_BUF, L"INSERT OR REPLACE INTO snapshot_reg_hklm_run(path, value, type, name) VALUES('%s', '%s', %ld, '%s'); ", reg[i].path, reg[i].value, reg[i].type, reg[i].name);
			break;
		case DB_REG_HKLM_RUNONCE:
			reg = (TWTL_DB_REGISTRY*)data;
			StringCchPrintfW(sql, MAX_SQL_BUF, L"INSERT OR REPLACE INTO snapshot_reg_hklm_runonce(path, value, type, name) VALUES('%s', '%s', %ld, '%s'); ", reg[i].path, reg[i].value, reg[i].type, reg[i].name);
			break;
		case DB_REG_HKCU_RUN:
			reg = (TWTL_DB_REGISTRY*)data;
			StringCchPrintfW(sql, MAX_SQL_BUF, L"INSERT OR REPLACE INTO snapshot_reg_hkcu_run(path, value, type, name) VALUES('%s', '%s', %ld, '%s'); ", reg[i].path, reg[i].value, reg[i].type, reg[i].name);
			break;
		case DB_REG_HKCU_RUNONCE:
			reg = (TWTL_DB_REGISTRY*)data;
			StringCchPrintfW(sql, MAX_SQL_BUF, L"INSERT OR REPLACE INTO snapshot_reg_hkcu_runonce(path, value, type, name) VALUES('%s', '%s', %ld, '%s'); ", reg[i].path, reg[i].value, reg[i].type, reg[i].name);
			break;
		case DB_SERVICE:
			srv = (TWTL_DB_SERVICE*)data;
			StringCchPrintfW(sql, MAX_SQL_BUF, L"INSERT OR REPLACE INTO snapshot_service(key, image_path) VALUES('%s', '%s'); ", srv[i].key, srv[i].image_path);
			break;
		case DB_NETWORK:
			net = (TWTL_DB_NETWORK*)data;
			StringCchPrintfW(sql, MAX_SQL_BUF, L"INSERT OR REPLACE INTO snapshot_network(src_ipv4, dest_ipv4, src_port, dest_port, pid, is_dangerous) VALUES(%lu, %lu, %hu, %hu, %hu, %hu); ", net[i].src_ipv4, net[i].dest_ipv4, net[i].src_port, net[i].dest_port, net[i].pid, net[i].is_dangerous);
			break;
		case DB_BLACKLIST:
			black = (TWTL_DB_BLACKLIST*)data;
			StringCchPrintfW(sql, MAX_SQL_BUF, L"INSERT OR REPLACE INTO snapshot_blacklist(image_path) VALUES('%s'); ", black[i].image_path);
			break;
		default:
			return FALSE;
		}

		sqlite3_stmt *stmt = NULL;

		// Prepare statement
		ret = sqlite3_prepare16_v2(db, sql, -1, &stmt, NULL);
		if (ret != SQLITE_OK) {
#ifdef _DEBUG
			fprintf(stderr, "[DB_Insert] sqlite3_prepare16_v2() failed: %s\n", sqlite3_errmsg(db));
#endif
			return FALSE;
		}

		// Run SQL
		ret = sqlite3_step(stmt);
		if (ret != SQLITE_DONE)
		{
			ret = sqlite3_finalize(stmt);
			return FALSE;
		}

		// Finalize Query
		ret = sqlite3_finalize(stmt);
		if (ret != SQLITE_OK) {
#ifdef _DEBUG
			fprintf(stderr, "[DB_Insert] sqlite3_finalize() failed: %s\n", sqlite3_errmsg(db));
#endif
			return FALSE;
		}
	}

	return TRUE;
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
TWTL_DATABASE_API BOOL __stdcall DB_Select(sqlite3 *db, DB_TABLE_TYPE type, void* data, int* count, WCHAR* sql_where)
{
	int ret = 0;
	BOOL countRow = (data == NULL) ? TRUE : FALSE;
	WCHAR sql[MAX_SQL_BUF] = { 0 };
	TWTL_DB_PROCESS* proc = NULL;
	TWTL_DB_REGISTRY* reg = NULL;
	TWTL_DB_SERVICE* srv = NULL;
	TWTL_DB_NETWORK* net = NULL;
	TWTL_DB_BLACKLIST* black = NULL;

	switch (type) {
	case DB_PROCESS:
		if (sql_where)
			StringCchPrintfW(sql, MAX_SQL_BUF, L"SELECT * from snapshot_process WHERE %s;", sql_where);
		else
			StringCchCopyW(sql, MAX_SQL_BUF, L"SELECT * from snapshot_process;");
		break;
	case DB_REG_HKLM_RUN:
		if (sql_where)
			StringCchPrintfW(sql, MAX_SQL_BUF, L"SELECT * from snapshot_reg_hklm_run WHERE %s;", sql_where);
		else
			StringCchCopyW(sql, MAX_SQL_BUF, L"SELECT * from snapshot_reg_hklm_run;");
		break;
	case DB_REG_HKLM_RUNONCE:
		if (sql_where)
			StringCchPrintfW(sql, MAX_SQL_BUF, L"SELECT * from snapshot_reg_hklm_runonce WHERE %s;", sql_where);
		else
			StringCchCopyW(sql, MAX_SQL_BUF, L"SELECT * from snapshot_reg_hklm_runonce;");
		break;
	case DB_REG_HKCU_RUN:
		if (sql_where)
			StringCchPrintfW(sql, MAX_SQL_BUF, L"SELECT * from snapshot_reg_hkcu_run WHERE %s;", sql_where);
		else
			StringCchCopyW(sql, MAX_SQL_BUF, L"SELECT * from snapshot_reg_hkcu_run;");
		break;
	case DB_REG_HKCU_RUNONCE:
		if (sql_where)
			StringCchPrintfW(sql, MAX_SQL_BUF, L"SELECT * from snapshot_reg_hkcu_runonce WHERE %s;", sql_where);
		else
			StringCchCopyW(sql, MAX_SQL_BUF, L"SELECT * from snapshot_reg_hkcu_runonce;");
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
	case DB_BLACKLIST:
		if (sql_where)
			StringCchPrintfW(sql, MAX_SQL_BUF, L"SELECT * from snapshot_blacklist WHERE %s;", sql_where);
		else
			StringCchCopyW(sql, MAX_SQL_BUF, L"SELECT * from snapshot_blacklist;");
	default:
		return FALSE;
	}

	sqlite3_stmt* stmt;
	// Prepare statement
	ret = sqlite3_prepare16_v2(db, sql, -1, &stmt, NULL);
	if (ret != SQLITE_OK) {
#ifdef _DEBUG
		fprintf(stderr, "[DB_Select] Failed to prepare statement: %s\n", sqlite3_errmsg(db));
#endif
		return FALSE;
	}

	// Run SQL
	int i = 0;
	while ((ret = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		if (countRow)
		{
			(*count)++;
		}
		else
		{
			switch (type) {
			case DB_PROCESS:
				proc = (TWTL_DB_PROCESS*)data;
				// sqlite3_column_int64(stmt, 0); is idx, nothing meaningless
				// proc[i].time = sqlite3_column_int64(stmt, 1);
				proc[i].pid = sqlite3_column_int(stmt, 2);
				proc[i].ppid = sqlite3_column_int(stmt, 3);
				StringCchCopyW(proc[i].process_name, DB_MAX_PROC_NAME, (const WCHAR*)sqlite3_column_text16(stmt, 4));
				StringCchCopyW(proc[i].process_path, MAX_PATH, (const WCHAR*)sqlite3_column_text16(stmt, 5));
				break;
			case DB_REG_HKLM_RUN:
			case DB_REG_HKLM_RUNONCE:
			case DB_REG_HKCU_RUN:
			case DB_REG_HKCU_RUNONCE:
				reg = (TWTL_DB_REGISTRY*)data;
				// sqlite3_column_int64(stmt, 0); is idx, nothing meaningless
				// reg[i].time = sqlite3_column_int64(stmt, 1);
				StringCchCopyW(reg[i].path, DB_MAX_REG_PATH, (const WCHAR*)sqlite3_column_text16(stmt, 2));
				StringCchCopyW(reg[i].value, DB_MAX_REG_VALUE, (const WCHAR*)sqlite3_column_text16(stmt, 3));
				reg[i].type = sqlite3_column_int(stmt, 4);
				StringCchCopyW(reg[i].name, DB_MAX_REG_NAME, (const WCHAR*)sqlite3_column_text16(stmt, 5));
				break;
			case DB_SERVICE:
				srv = (TWTL_DB_SERVICE*)data;
				// sqlite3_column_int64(stmt, 0); is idx, nothing meaningless
				// srv[i].time = sqlite3_column_int64(stmt, 1);
				StringCchCopyW(srv[i].key, DB_MAX_REG_PATH, (const WCHAR*)sqlite3_column_text16(stmt, 2));
				break;
			case DB_NETWORK:
				net = (TWTL_DB_NETWORK*)data;
				// sqlite3_column_int64(stmt, 0); is idx, nothing meaningless
				// net[i].time = sqlite3_column_int64(stmt, 1);
				net[i].src_ipv4 = (uint32_t)sqlite3_column_int(stmt, 2);
				net[i].dest_ipv4 = (uint32_t)sqlite3_column_int(stmt, 3);
				net[i].src_port = (uint16_t)sqlite3_column_int(stmt, 4);
				net[i].dest_port = (uint16_t)sqlite3_column_int(stmt, 5);
				net[i].pid = (uint16_t)sqlite3_column_int(stmt, 6);
				net[i].is_dangerous = (uint16_t)sqlite3_column_int(stmt, 7);
				break;
			case DB_BLACKLIST:
				black = (TWTL_DB_BLACKLIST*)data;
				// sqlite3_column_int64(stmt, 0); is idx, nothing meaningless
				// black[i].time = sqlite3_column_int64(stmt, 1);
				StringCchCopyW(black[i].image_path, DB_MAX_FILE_PATH, (const WCHAR*)sqlite3_column_text16(stmt, 2));
				break;
			default:
				break; // Do nothing
			}
			i++;
		}
	}
	
	ret = sqlite3_finalize(stmt);
	if (ret != SQLITE_OK) {
#ifdef _DEBUG
		fprintf(stderr, "[DB_Select] Failed to fetch data: %s\n", sqlite3_errmsg(db));
#endif
		return FALSE;
	}

	return TRUE;
}

/*
Description : Jumo, drop the table entries!
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
TWTL_DATABASE_API BOOL __stdcall DB_Delete(sqlite3 *db, DB_TABLE_TYPE type, WCHAR* sql_where)
{
	int ret = 0;

	WCHAR sql[MAX_SQL_BUF] = { 0 };

	switch (type) {
	case DB_PROCESS:
		if (sql_where)
			StringCchPrintfW(sql, MAX_SQL_BUF, L"DELETE FROM snapshot_process WHERE %s; ", sql_where);
		else
			StringCchPrintfW(sql, MAX_SQL_BUF, L"DELETE FROM snapshot_process; ");
		break;
	case DB_REG_HKLM_RUN:
		if (sql_where)
			StringCchPrintfW(sql, MAX_SQL_BUF, L"DELETE FROM snapshot_reg_hklm_run WHERE %s; ", sql_where);
		else
			StringCchPrintfW(sql, MAX_SQL_BUF, L"DELETE FROM snapshot_reg_hklm_run; ");
		break;
	case DB_REG_HKLM_RUNONCE:
		if (sql_where)
			StringCchPrintfW(sql, MAX_SQL_BUF, L"DELETE FROM snapshot_reg_hklm_runonce WHERE %s; ", sql_where);
		else
			StringCchPrintfW(sql, MAX_SQL_BUF, L"DELETE FROM snapshot_reg_hklm_runonce; ");
		break;
	case DB_REG_HKCU_RUN:
		if (sql_where)
			StringCchPrintfW(sql, MAX_SQL_BUF, L"DELETE FROM snapshot_reg_hkcu_run WHERE %s; ", sql_where);
		else
			StringCchPrintfW(sql, MAX_SQL_BUF, L"DELETE FROM snapshot_reg_hkcu_run; ");
		break;
	case DB_REG_HKCU_RUNONCE:
		if (sql_where)
			StringCchPrintfW(sql, MAX_SQL_BUF, L"DELETE FROM snapshot_reg_hkcu_runonce WHERE %s; ", sql_where);
		else
			StringCchPrintfW(sql, MAX_SQL_BUF, L"DELETE FROM snapshot_reg_hkcu_runonce; ");
		break;
	case DB_SERVICE:
		if (sql_where)
			StringCchPrintfW(sql, MAX_SQL_BUF, L"DELETE FROM snapshot_service WHERE %s; ", sql_where);
		else
			StringCchPrintfW(sql, MAX_SQL_BUF, L"DELETE FROM snapshot_service; ");
		break;
	case DB_NETWORK:
		if (sql_where)
			StringCchPrintfW(sql, MAX_SQL_BUF, L"DELETE FROM snapshot_network WHERE %s; ", sql_where);
		else
			StringCchPrintfW(sql, MAX_SQL_BUF, L"DELETE FROM snapshot_network; ");
		break;
	case DB_BLACKLIST:
		if (sql_where)
			StringCchPrintfW(sql, MAX_SQL_BUF, L"DELETE FROM snapshot_blacklist WHERE %s; ", sql_where);
		else
			StringCchPrintfW(sql, MAX_SQL_BUF, L"DELETE FROM snapshot_blacklist; ");
		break;
	default:
		return FALSE;
	}

	sqlite3_stmt *stmt = NULL;

	// Prepare statement
	ret = sqlite3_prepare16_v2(db, sql, -1, &stmt, NULL);
	if (ret != SQLITE_OK) {
#ifdef _DEBUG
		fprintf(stderr, "[DB_Delete] sqlite3_prepare16_v2() failed: %s\n", sqlite3_errmsg(db));
#endif
		return FALSE;
	}

	// Run SQL
	ret = sqlite3_step(stmt);
	if (ret != SQLITE_DONE)
	{
#ifdef _DEBUG
		fprintf(stderr, "[DB_Delete] sqlite3_step() failed: %s\n", sqlite3_errmsg(db));
#endif
		ret = sqlite3_finalize(stmt);
		return FALSE;
	}

	/*
	ret = sqlite3_reset(stmt);
	if (ret != SQLITE_OK)
	{
#ifdef _DEBUG
		fprintf(stderr, "[DB_Delete] sqlite3_reset() failed: %s\n", sqlite3_errmsg(db));
#endif
		return FALSE;
	}
	*/

	// Finalize Query
	ret = sqlite3_finalize(stmt);
	if (ret != SQLITE_OK) {
#ifdef _DEBUG
		fprintf(stderr, "[DB_Delete] sqlite3_finalize() failed: %s\n", sqlite3_errmsg(db));
#endif
		return FALSE;
	}

	return TRUE;
}