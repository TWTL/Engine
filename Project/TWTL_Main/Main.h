#pragma once
#pragma comment(lib, "TWTL_Snapshot.lib")

#define PRONAME_MAX	 260
#define REGNAME_MAX	 255
#define REGVALUE_MAX 16383

__declspec(dllimport)
BOOL
__stdcall
snapCurrentStatus(
	CONST DWORD32 mode
);

__declspec(dllimport)
BOOL
__stdcall
deleteRunKey(
	TCHAR CONST keyName[REGNAME_MAX],
	CONST DWORD32 targetKey
);

__declspec(dllimport)
VOID
__stdcall
delayWait(
	CONST DWORD dwMillisecond
);

__declspec(dllimport)
VOID
__stdcall
errMsg(
);