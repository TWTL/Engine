#pragma once

#include "stdafx.h"
#include "Misc.h"
#include "NetMonitor.h"

TWTL_SNAPSHOT_API
BOOL
__stdcall
SnapCurrentStatus(
	CONST DWORD32 mode
);

TWTL_SNAPSHOT_API
BOOL
__stdcall
TerminateCurrentProcess(
	CONST DWORD32 targetPID,
	CONST DWORD mode
);

TWTL_SNAPSHOT_API
BOOL
__stdcall 
DeleteRunKey(
	TCHAR CONST keyName[REGNAME_MAX],
	CONST DWORD32 targetKey
);