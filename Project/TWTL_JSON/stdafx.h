// stdafx.h : 자주 사용하지만 자주 변경되지는 않는
// 표준 시스템 포함 파일 또는 프로젝트 관련 포함 파일이
// 들어 있는 포함 파일입니다.
//

#pragma once

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ws2_32.lib")

#ifdef _DEBUG
#pragma comment(lib, "..\\lib\\jansson_d.lib")
#else
#pragma comment(lib, "..\\lib\\jansson.lib")
#endif

#include "Var.h"

#include "targetver.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define WIN32_LEAN_AND_MEAN             // 거의 사용되지 않는 내용은 Windows 헤더에서 제외합니다.
// Windows 헤더 파일:
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <strsafe.h>
#include <shlwapi.h>

#include "..\include\jansson.h"

// TODO: 프로그램에 필요한 추가 헤더는 여기에서 참조합니다.
