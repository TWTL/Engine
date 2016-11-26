#pragma once

#include "stdafx.h"

#include "JsonFunc.h"

typedef unsigned int (WINAPI *LPTHREADPROC)(LPVOID lpParam);

DWORD __stdcall SOCK_MainPortInit();
DWORD __stdcall SOCK_MainPortProc();
DWORD __stdcall SOCK_MainPortClose();

DWORD SOCK_MainPortResponse(TWTL_PROTO_BUF *req, SOCKET socket);
BOOL SOCK_SendProtoBuf(SOCKET sock, TWTL_PROTO_BUF *buf);

DWORD SOCK_TrapPortInit(LPCSTR address, LPCSTR port);
DWORD SOCK_TrapPortProc();
DWORD SOCK_TrapPortClose();