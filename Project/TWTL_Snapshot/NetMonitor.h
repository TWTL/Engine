#pragma once

#include "stdafx.h"
#include "MonitorFunc.h"

#define addr_size (3 + 3*4 + 1)   // xxx.xxx.xxx.xxx\0

BOOL
__stdcall
ParseNetstat(
	TWTL_DB_NETWORK* sqliteNet1,
	TWTL_DB_NETWORK* sqliteNet2,
	DWORD structSize[],
	CONST DWORD32 mode
);