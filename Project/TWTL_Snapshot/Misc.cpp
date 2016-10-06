// Misc.cpp : Other functions used for some objective.
// #define _CRT_SECURE_NO_WARNINGS

#include "stdafx.h"
#include "Misc.h"

/*
	Description : Get privilege for using windows APIs.

	Parameters :
		( out ) void hProcess = Get Privileged Handle
	Return value :
		0 = Error
		1 = Success
*/
BOOL __stdcall setSystemPrivilege(HANDLE *hProcess) {
	LUID setDebug;
	TOKEN_PRIVILEGES tokenPri;

	if (!OpenProcessToken(GetCurrentProcess(),
		TOKEN_ADJUST_PRIVILEGES | TOKEN_ALL_ACCESS,
		hProcess))
	{
		return NULL;
	}

	if (!LookupPrivilegeValue(NULL,
		SE_DEBUG_NAME,
		&setDebug))
	{
		return NULL;
	}
	tokenPri.PrivilegeCount = 1;
	tokenPri.Privileges[0].Luid = setDebug;
	tokenPri.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if (!AdjustTokenPrivileges(*hProcess,
		FALSE,
		&tokenPri,
		sizeof(tokenPri),
		NULL,
		NULL))
	{
		return NULL;
	}

	return TRUE;
}

/*
	Description : Initialize values of structure reg.

	Parameters : 
		( out ) PREGINFO reg = initialized by this function.
	Return value :
		0 = Error
		1 = Success
*/
BOOL __stdcall initRegSize(CONST PREGINFO reg) {
	reg->bufSize = REG_MAX - 1;
	reg->dataType = REG_SZ;
	
	if (reg->bufSize != REG_MAX - 1 || reg->dataType != REG_SZ) {
		return NULL;
	}
	return TRUE;
}

/*
	Description : Write name of process snapshot.
				  ( This function may be deprecated if we'll use DB. )

	Parameters :
		( out ) char *fileName = Name of process snapshot
		( snapshot-yyyy-mm-dd-hh-mm-ss.txt )
	Return value :
		0 = Error
		1 = Success
*/
BOOL __stdcall makeFileName(CHAR fileName[]) {
	time_t timer;
	tm t;
	CHAR fileNameBuf[5] = { 0 };
	DWORD32 timeBuf = 0;
	
	// _itoa_s ,strcat_s and localtime_s are errno_t.
	// so these functions return 0 if successful.
	//

	timer = time(NULL);
	if (localtime_s(&t, &timer)) {
		return NULL;
	}
	
	for (int i = 0; i < 6; i++)
	{
		if (i == 0) {
			timeBuf = t.tm_year + 1900;
		}
		else if (i == 1) {
			timeBuf = t.tm_mon + 1;
		}
		else if (i == 2) {
			timeBuf = t.tm_mday;
		}
		else if (i == 3) {
			timeBuf = t.tm_hour;
		}
		else if (i == 4) {
			timeBuf = t.tm_min;
		}
		else if (i == 5) {
			timeBuf = t.tm_sec;
		}
		
		if (_itoa_s(timeBuf, fileNameBuf, 10)) {
			return NULL;
		}

		if (strcat_s(fileName, 
			MAX_PATH,
			"-")) 
		{
			return NULL;
		}

		// this branch need to fix name
		// ( e.g. snapshot-yyyy-mm-dd-hh-mm-ss.txt )
		//

		if (fileNameBuf[1] == NULL) {
			if (strcat_s(fileName,
				MAX_PATH,
				"0"))
			{
				return NULL;
			}
		}

		if (strcat_s(fileName, 
			MAX_PATH,
			fileNameBuf)) 
		{
			return NULL;
		}
	}
	if (strcat_s(fileName, 
		MAX_PATH,
		".txt")) 
	{
		return NULL;
	}
	return TRUE;
}

/*
	Description : Instead of use sleep(), use this function.

	Parameters :	Nope
	Return value :  Nope
*/
VOID __stdcall delayWait(CONST DWORD dwMillisecond) {
	MSG msg;
	DWORD dwStart;
	dwStart = GetTickCount();

	while (GetTickCount() - dwStart < dwMillisecond)
	{
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return;
}

/*
	Description : Print written string. ( for debugging )

	Parameters :	Nope
	Return value :	Nope
*/
VOID __stdcall printCUI(CONST TCHAR* msg) {
	_tprintf_s(L"Written : %s\n", msg);
	return;
}

/*
	Description : Print error message. ( for debugging )

	Parameters :	Nope
	Return value :	Nope
*/
TWTL_SNAPSHOT_API VOID __stdcall errMsg() {

	WCHAR errMsg[40] = { 0 };
	DWORD const Errno = GetLastError();

	// Error Case
	_wcserror_s(errMsg, 40, Errno);

	_tprintf_s(L"Error %d : %s\n", Errno, errMsg);
	return;
}