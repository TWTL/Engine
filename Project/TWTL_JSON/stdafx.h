// stdafx.h : ���� ��������� ���� ��������� �ʴ�
// ǥ�� �ý��� ���� ���� �Ǵ� ������Ʈ ���� ���� ������
// ��� �ִ� ���� �����Դϴ�.
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


#define WIN32_LEAN_AND_MEAN             // ���� ������ �ʴ� ������ Windows ������� �����մϴ�.
// Windows ��� ����:
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <strsafe.h>
#include <shlwapi.h>

#include "..\include\jansson.h"

// TODO: ���α׷��� �ʿ��� �߰� ����� ���⿡�� �����մϴ�.

void BinaryDump(const uint8_t buf[], const uint32_t bufsize);
