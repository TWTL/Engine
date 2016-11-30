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
#include "..\TWTL_Database\Database.h"

#define PRONAME_MAX	 260
#define REGNAME_MAX	 255
#define REGVALUE_MAX 16383

#ifndef TWTL_TRAP_H
#define TWTL_TRAP_H
#define TRAP_PATH_MAX 1024
typedef struct twtl_trap_queue_node {
	char path[TRAP_PATH_MAX];
	struct twtl_trap_queue_node* next;
} TWTL_TRAP_QUEUE_NODE;
typedef struct twtl_trap_queue {
	int count;
	struct twtl_trap_queue_node* node;
} TWTL_TRAP_QUEUE;
typedef BOOL(*JSON_EnqTrapQueue_t)(TWTL_TRAP_QUEUE* queue, char* inPath);
#endif

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
	DWORD structSize[],
	CONST DWORD32 mode,
	JSON_EnqTrapQueue_t trapProc,
	TWTL_TRAP_QUEUE* queue
);

__declspec(dllimport)
BOOL
__stdcall
TerminateCurrentProcess(
	CONST DWORD32 targetPID,
	TCHAR imagePath[],
	TCHAR(*blackList)[MAX_PATH],
	CONST DWORD length,
	CONST DWORD mode
);

__declspec(dllimport)
BOOL
__stdcall
DeleteKeyOrKeyValue(
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


