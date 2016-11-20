#pragma once

#include "stdafx.h"

typedef unsigned int (WINAPI *LPTHREADPROC)(LPVOID lpParam);

TWTL_JSON_API DWORD __stdcall JSON_InitMainSocket();
TWTL_JSON_API DWORD __stdcall JSON_ProcMainSocket(BOOL* quitSignal);
TWTL_JSON_API DWORD __stdcall JSON_CloseMainSocket();

TWTL_JSON_API DWORD __stdcall JSON_InitTrapSocket(LPCSTR address, LPCSTR port);
TWTL_JSON_API DWORD __stdcall JSON_ProcTrapSocket(BOOL* quitSignal);
TWTL_JSON_API DWORD __stdcall JSON_CloseTrapSocket();