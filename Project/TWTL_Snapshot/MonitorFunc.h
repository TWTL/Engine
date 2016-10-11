#pragma once

#include "stdafx.h"
#include "Misc.h"

TWTL_SNAPSHOT_API
BOOL
__stdcall
snapCurrentStatus(
	CONST DWORD32 mode
);

TWTL_SNAPSHOT_API
BOOL
__stdcall 
deleteRunKey(
	TCHAR CONST keyName[REGNAME_MAX],
	CONST DWORD32 targetKey
);