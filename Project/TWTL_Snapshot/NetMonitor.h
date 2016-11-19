#pragma once

#include "stdafx.h"

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

#define addr_size (3 + 3*4 + 1)   // xxx.xxx.xxx.xxx\0

BOOL
__stdcall
ParseNetstat(
);