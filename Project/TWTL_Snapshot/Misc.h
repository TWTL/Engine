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
SetSystemPrivilege(
	HANDLE *hProcess
);

BOOL
__stdcall
InitRegSize(
	PREGINFO CONST reg,
	CONST DWORD32  regType
);

BOOL
__stdcall
MakeFileName(
	CHAR fileName[]
);

VOID
__stdcall
PrintCUI(
	TCHAR* CONST msg
);

TWTL_SNAPSHOT_API
VOID
__stdcall
DelayWait(
	CONST DWORD dwMillisecond
);

TWTL_SNAPSHOT_API
VOID
__stdcall
ErrMsg(
);