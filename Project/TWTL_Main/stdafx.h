// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#ifdef _DEBUG
#pragma comment(lib, "..\\lib\\jansson_d.lib")
#else
#pragma comment(lib, "..\\lib\\jansson.lib")
#endif



#include "targetver.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <tchar.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <process.h>
#include <ws2tcpip.h>
#include <strsafe.h>
#include <shlwapi.h>

// TODO: reference additional headers your program requires here

#include "Main.h"
#include "..\include\jansson.h"

#define TWTL_JSON_MAX_BUF	2048
#define TWTL_JSON_PORT		5259
#define TWTL_JSON_PORT_WSTR	L"5259"