#pragma once

#include "stdafx.h"

typedef unsigned int (WINAPI *LPTHREADPROC)(LPVOID lpParam);

DWORD __stdcall JSON_InitMainSocket();
DWORD __stdcall JSON_ProcMainSocket(BOOL* quitSignal);
DWORD __stdcall JSON_CloseMainSocket();

DWORD __stdcall JSON_InitTrapSocket(LPCSTR address, LPCSTR port);
DWORD __stdcall JSON_ProcTrapSocket(BOOL* quitSignal);
DWORD __stdcall JSON_CloseTrapSocket();