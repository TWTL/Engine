#pragma once

#include "stdafx.h"

enum {
	INJECTION_MODE = 0,
	EJECTION_MODE
};

BOOL 
__stdcall 
InjectDll(
	DWORD dwPID, 
	LPCTSTR szDllPath
);

BOOL 
__stdcall 
EjectDll(
	DWORD dwPID, 
	LPCTSTR szDllPath
);

BOOL 
__stdcall 
InjectAllProcess(
	DWORD nMode, 
	LPCTSTR szDllPath
);
