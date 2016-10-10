#pragma once

#include "stdafx.h"

#ifdef TWTL_SNAPSHOT_EXPORTS
#define TWTL_SNAPSHOT_API __declspec(dllexport) 
#else
#define TWTL_SNAPSHOT_API __declspec(dllimport) 
#endif

#define REGNAME_MAX	 255
#define REGVALUE_MAX 16383

typedef struct REG_INFORMATION
{
	HKEY  key;
	TCHAR keyName[REGNAME_MAX];
	TCHAR keyValue[REGVALUE_MAX];
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
	PREGINFO CONST reg,
	CONST DWORD32  regType
);

BOOL
__stdcall
makeFileName(
	CHAR fileName[]
);

VOID
__stdcall
printCUI(
	TCHAR* CONST msg
);

TWTL_SNAPSHOT_API
VOID
__stdcall
delayWait(
	CONST DWORD dwMillisecond
);

TWTL_SNAPSHOT_API
VOID
__stdcall
errMsg(
);