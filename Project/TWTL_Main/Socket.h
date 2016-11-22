#pragma once

#include "stdafx.h"

typedef unsigned int (WINAPI *LPTHREADPROC)(LPVOID lpParam);

DWORD __stdcall SOCK_MainPortInit();
DWORD __stdcall SOCK_MainPortProc();
DWORD __stdcall SOCK_MainPortClose();

DWORD __stdcall SOCK_TrapPortInit(LPCSTR address, LPCSTR port);
DWORD __stdcall JSON_ProcTrapSocket();
DWORD __stdcall SOCK_TrapPortClose();