// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <time.h>
#include <string.h>

#include <Windows.h>
#include <Psapi.h>
#include <TlHelp32.h>
#include <iphlpapi.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <Shlwapi.h>

#include <inttypes.h>
#include <stdint.h>
#include <atlstr.h>

#include <iostream>

// TODO: reference additional headers your program requires here