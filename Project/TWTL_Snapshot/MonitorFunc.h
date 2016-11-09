#pragma once

#include "stdafx.h"
#include "Misc.h"

TWTL_SNAPSHOT_API
BOOL
__stdcall
SnapCurrentStatus(
	CONST DWORD32 mode
);

TWTL_SNAPSHOT_API
BOOL
__stdcall 
DeleteRunKey(
	TCHAR CONST keyName[REGNAME_MAX],
	CONST DWORD32 targetKey
);