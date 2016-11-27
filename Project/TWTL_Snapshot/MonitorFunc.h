#pragma once

#ifdef _DEBUG
#pragma comment(lib, "..\\Debug\\TWTL_Database.lib")
#else
#pragma comment(lib, "..\\Release\\TWTL_Database.lib")
#endif

#include "stdafx.h"
#include "Misc.h"
#include "..\TWTL_Database\sqlite3.h"
#include "..\TWTL_Database\Database.h"

#include "NetMonitor.h"
#include <Shlwapi.h>

TWTL_SNAPSHOT_API
BOOL
__stdcall
SnapCurrentStatus(
	TWTL_DB_PROCESS*  sqlitePrc,  // Result of parsing PROCESSENTRY32W
	TWTL_DB_REGISTRY* sqliteReg1, // HKCU - Run
	TWTL_DB_REGISTRY* sqliteReg2, // HKLM - Run
	TWTL_DB_REGISTRY* sqliteReg3, // HKCU - RunOnce
	TWTL_DB_REGISTRY* sqliteReg4, // HKLM - RunOnce
	TWTL_DB_SERVICE*  sqliteSvc,  // Result of parsing Services
	TWTL_DB_NETWORK*  sqliteNet1, // TCP
	TWTL_DB_NETWORK*  sqliteNet2, // UDP
	DWORD structSize[],
	CONST DWORD32 mode
);

TWTL_SNAPSHOT_API
BOOL
__stdcall
TerminateCurrentProcess(
	CONST DWORD32 targetPID,
	TCHAR imagePath[],
	TCHAR(*blackList)[MAX_PATH],
	CONST DWORD length,
	CONST DWORD mode
);

TWTL_SNAPSHOT_API
BOOL
__stdcall 
DeleteKeyOrKeyValue(
	TCHAR CONST keyName[REGNAME_MAX],
	CONST DWORD32 targetKey
);