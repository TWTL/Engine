// MonitorFunc.cpp : Functions of monitor.

#include "stdafx.h"
#include "MonitorFunc.h"

BOOL __stdcall setTargetRegistryEntry(FILE* storage, CONST PREGINFO pReg, CONST DWORD target);
BOOL __stdcall writeRegToTxt(FILE* storage, CONST PREGINFO pReg);

/*
	Description : Write current process entry and register ( Run ) 
				  in text file. ( txt export )

	Parameters : Nope
	Return value :
		0 = Error
		1 = Success
*/
TWTL_SNAPSHOT_API BOOL __stdcall snapCurrentStatus() {
	FILE* storage;

	HANDLE hSnap;
	PROCESSENTRY32 proc32;

	LPWSTR Pname = { 0, };
	CHAR fileName[MAX_PATH] = "Snapshot";

	REGINFO reg;
	PREGINFO pReg = &reg;
	reg.bufSize = REG_MAX - 1;
	reg.dataType = REG_SZ;

	if (!makeFileName(fileName)) {
		return NULL;
	}

	if (fopen_s(&storage, fileName, "w")) {
		return NULL;
	}
	if ((hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0))
		== INVALID_HANDLE_VALUE) {
		fclose(storage);
		return NULL;
	}

	else {
		proc32.dwSize = sizeof(PROCESSENTRY32);
		DWORD32 PID = 0;
		if (Process32First(hSnap, &proc32)) {
			fwprintf_s(storage, L"<Process List>\n");

			// First Process' ID'll be 0, 
			// so if calling Process32Next before writing, it'll be valid process.
			//

			while ((Process32Next(hSnap, &proc32)))
			{
				printCUI(proc32.szExeFile);
				fwprintf_s(storage, L"%s\n", &proc32.szExeFile);
			}
			fwprintf_s(storage, L"</Process List>\n\n");
		}
		CloseHandle(hSnap);
	}

	// Write value of register key value ( Run of current user )
	//

	if (!setTargetRegistryEntry(storage, pReg, 1)) {
		return NULL;
	}
	if (!setTargetRegistryEntry(storage, pReg, 2)) {
		return NULL;
	}

	fclose(storage);
	return TRUE;
}

/*
	Description : delete selected key ( ~/Run )

	Parameter :
		( in ) : 
*/
BOOL __stdcall deleteRunKey() {
	return TRUE;
}

/*
	Description : set target registry entry

	Parameter :
		( out ) FILE* storage : written txt file
		( in )	PREGINFO pReg : information of target entry
		( in )	DWORD target  : 
			1 -> HKEY_CURRENT_USER/Microsoft/Windows/CurrentVersion/Run
			2 -> HKEY_LOCAL_MACHINE/Microsoft/Windows/CurrentVersion/Run
			else -> Error
	Return value :
		0 = Error
		1 = Success
*/
BOOL __stdcall setTargetRegistryEntry(FILE* storage, CONST PREGINFO pReg, CONST DWORD target) {
	HKEY mainEntry;
	
	if (target == 1) {
		mainEntry = HKEY_CURRENT_USER;
	}
	else if (target == 2) {
		mainEntry = HKEY_LOCAL_MACHINE;
	}
	else {
		return NULL;
	}

	if (RegOpenKeyEx(mainEntry,
		L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
		NULL,
		KEY_ALL_ACCESS,
		&pReg->key)
		== 0)
	{
		if (target == 1) {
			fwprintf_s(storage, L"<Register List ( for current users )>\n");
		}
		else if (target == 2) {
			fwprintf_s(storage, L"<Register List ( for all users )>\n");
		}

		// call writing entry of target
		//
		if (!writeRegToTxt(storage, pReg)) {
			return NULL;
		}
		
		if (target == 1) {
			fwprintf_s(storage, L"</Register List ( for current users )>\n\n");
		}
		else if (target == 2) {
			fwprintf_s(storage, L"</Register List ( for all users )>\n\n");
		}

		RegCloseKey(pReg->key);
	}
	else {
		fwprintf_s(storage, L"Can't get register key. :(\n");
		fclose(storage);
		return NULL;
	}

	return TRUE;
}

/*
	Description : Write register value in text file. ( txt export )

	Parameters : 
		( out ) FILE* storage : written by this function.
		( in )	PREGINFO pReg : information of target register entry.
	Return value :
		0 = Error
		1 = Success
*/
BOOL __stdcall writeRegToTxt(FILE* storage, CONST PREGINFO pReg) {
	DWORD result = 0;

	for (int i = 0; result == ERROR_SUCCESS; i++)
	{
		result = RegEnumValue(pReg->key, i, pReg->keyName, &pReg->bufSize, NULL, NULL, NULL, NULL);

		if (result == ERROR_SUCCESS)
		{
			// must initialize reg.bufsize
			//
			if (!initRegSize(pReg)) {
				return NULL;
			}

			if (RegQueryValueEx(pReg->key,
				pReg->keyName,
				NULL,
				&pReg->dataType,
				(LPBYTE)pReg->keyValue,
				&pReg->bufSize)
				!= 0)
			{
				return NULL;
			}
			// must initialize reg.bufsize
			//
			if (!initRegSize(pReg)) {
				return NULL;
			}
			printCUI(pReg->keyName);
			printCUI(pReg->keyValue);
			fwprintf_s(storage, L"%s, ", pReg->keyName);
			fwprintf_s(storage, L"%s\n", pReg->keyValue);
		}
	}
	return TRUE;
}