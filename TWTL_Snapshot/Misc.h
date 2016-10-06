#pragma once

#include "stdafx.h"

#ifdef TWTL_SNAPSHOT_EXPORTS
#define TWTL_SNAPSHOT_API __declspec(dllexport) 
#else
#define TWTL_SNAPSHOT_API __declspec(dllimport) 
#endif

#define REG_MAX 255

typedef struct REG_INFORMATION
{
	HKEY key;
	TCHAR keyName[REG_MAX];
	TCHAR keyValue[REG_MAX];
	DWORD bufSize;
	DWORD dataType;
}REGINFO, *PREGINFO;

BOOL 
__stdcall
setSystemPrivilege(
	HANDLE *hProcess
);

BOOL
__stdcall
initRegSize(
	CONST PREGINFO reg
);

BOOL
__stdcall
makeFileName(
	CHAR fileName[]
);

VOID
__stdcall
delayWait(
	CONST DWORD dwMillisecond
);

VOID
__stdcall
printCUI(
	CONST TCHAR* msg
);

TWTL_SNAPSHOT_API
VOID
__stdcall
errMsg(
);