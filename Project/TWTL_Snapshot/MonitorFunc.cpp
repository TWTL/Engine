// MonitorFunc.cpp : Functions of monitor.

#include "stdafx.h"
#include "MonitorFunc.h"

DWORD __stdcall OpenRegisteryKey(PREGINFO CONST pReg, CONST DWORD32 target);
BOOL __stdcall SetTargetRegistryEntry(FILE* storage, PREGINFO CONST pReg, TWTL_DB_REGISTRY* sqliteReg, TWTL_DB_SERVICE* sqliteSvc ,CONST DWORD32 target, CONST DWORD32 mode);
BOOL __stdcall WriteRegToTxt(FILE* storage, PREGINFO CONST pReg, TWTL_DB_REGISTRY* sqliteReg, TWTL_DB_SERVICE* sqliteSvc, CONST DWORD32 mode, CONST DWORD32 target);

/*
	Description : Write current process entry and register ( Run ) 
				  in text file. ( txt export )

	Parameters : 
		( out ) TWTL_DB_PROCESS* sqlitePrc : Result of parsing PROCESSENTRY32W
		( in )	DWORD32 mode :
			0 -> just print
			1 -> txt export
			else -> error
	Return value :
		0 = Error
		1 = Success
*/
TWTL_SNAPSHOT_API BOOL __stdcall SnapCurrentStatus(
	TWTL_DB_PROCESS*  sqlitePrc,
	TWTL_DB_REGISTRY* sqliteReg1,
	TWTL_DB_REGISTRY* sqliteReg2,
	TWTL_DB_REGISTRY* sqliteReg3,
	TWTL_DB_REGISTRY* sqliteReg4,
	TWTL_DB_SERVICE*  sqliteSvc,
	TWTL_DB_NETWORK*  sqliteNet1,
	TWTL_DB_NETWORK*  sqliteNet2,
	CONST DWORD32 mode) 
{
	FILE* storage = NULL;

	CPROINFO currentProcessInfo;
	PCPROINFO pCurrentProcessInfo = &currentProcessInfo;
	currentProcessInfo.pProc32 = &currentProcessInfo.proc32;

	// use c++ because of readability
	LPWSTR imageName = new WCHAR[MAX_PATH];
	DWORD imageNameSize = MAX_PATH;

	CHAR fileName[MAX_PROC_NAME] = "Snapshot";
	TCHAR curPID[PROPID_MAX] = { 0, };

	REGINFO reg;
	PREGINFO pReg = &reg;
	reg.bufSize = REGNAME_MAX - 1;
	reg.dataType = REG_SZ;

	int i = 0;
	int listNum = 0;

	SetPrivilege(SE_DEBUG_NAME, TRUE);

	if (!MakeFileName(fileName)) {
		return NULL;
	}
	if (mode == 0) {

	}
	else if (mode == 1) {
		if (fopen_s(&storage, fileName, "w")) {
			return NULL;
		}
	}
	else if (mode == 2) {

	}
	else {
		return NULL;
	}

	if ((currentProcessInfo.hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0))
		== INVALID_HANDLE_VALUE) {
		ExceptionFileClose(storage, mode);
		return NULL;
	}

	currentProcessInfo.proc32.dwSize = sizeof(PROCESSENTRY32);
	DWORD32 PID = 0;
	if (Process32First(currentProcessInfo.hSnap, &currentProcessInfo.proc32)) {
		listNum = 0;
		
		if (mode == 1) {
			fwprintf_s(storage, L"<Process List>\n");
		}
		// First Process' ID'll be 0, 
		// so if calling Process32Next before writing, it'll be valid process.
		//
		while (Process32Next(currentProcessInfo.hSnap, &currentProcessInfo.proc32))
		{
			if (currentProcessInfo.proc32.th32ProcessID == 4) {

			}
			else {
				listNum++;
			}
		}
		if (Process32First(currentProcessInfo.hSnap, &currentProcessInfo.proc32)) {
			DWORD result = 0;
			i = 0;
			sqlitePrc = (TWTL_DB_PROCESS*)realloc(sqlitePrc, sizeof(TWTL_DB_PROCESS)*(listNum+10));

			while ((Process32Next(currentProcessInfo.hSnap, &currentProcessInfo.proc32)))
			{
				if (currentProcessInfo.proc32.th32ProcessID < 100) {
					continue;
				}
				else {
					currentProcessInfo.curHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, currentProcessInfo.proc32.th32ProcessID);
					if (currentProcessInfo.curHandle != NULL) {
						result = QueryFullProcessImageName(currentProcessInfo.curHandle, 0, imageName, &imageNameSize);
						imageNameSize = MAX_PATH;
						if (result == 0) {
							ErrMsg();
							_tprintf_s(L"Nope :( ... Maybe system file. ");
						}
						else {
							wcscpy_s(sqlitePrc[i].process_path, MAX_PATH, imageName);
							wcscpy_s(sqlitePrc[i].process_name, DB_MAX_PROC_NAME, currentProcessInfo.proc32.szExeFile);
							sqlitePrc[i].pid = currentProcessInfo.proc32.th32ProcessID;
							sqlitePrc[i].ppid = currentProcessInfo.proc32.th32ParentProcessID;

							if (mode == 0) {
								_tprintf_s(L"%s ", sqlitePrc[i].process_path);
								_tprintf_s(L"Process name : %s, PID : %d, PPID :%d\n",
									currentProcessInfo.proc32.szExeFile,
									currentProcessInfo.proc32.th32ProcessID,
									currentProcessInfo.proc32.th32ParentProcessID);
							}
							else if (mode == 1) {
								if (_itow_s(currentProcessInfo.proc32.th32ProcessID, curPID, 5, 10)) {
									ExceptionFileClose(storage, mode);
									return NULL;
								}
								fwprintf_s(storage, L"%s\t", &curPID);
								fwprintf_s(storage, L"%s\n", &currentProcessInfo.proc32.szExeFile);
								fwprintf_s(storage, L"%s\t", imageName);
							}
						}
						CloseHandle(currentProcessInfo.curHandle);
					}
				}
				i++;
			}
		}
		if (mode == 1) {
			fwprintf_s(storage, L"</Process List>\n\n");
		}
	}
	CloseHandle(currentProcessInfo.hSnap);
	

	// Write value of register key value ( Run of current user )
	//

	if (!SetTargetRegistryEntry(storage, pReg, sqliteReg1, NULL, 1, mode)||
		!SetTargetRegistryEntry(storage, pReg, sqliteReg2, NULL, 2, mode)||
		!SetTargetRegistryEntry(storage, pReg, sqliteReg3, NULL, 3, mode)||
		!SetTargetRegistryEntry(storage, pReg, sqliteReg4, NULL, 4, mode)||
		!SetTargetRegistryEntry(storage, pReg, NULL, sqliteSvc, 5, mode))
	{
		ExceptionFileClose(storage, mode);
		return NULL;
	}
	if (mode == 1) {
		fclose(storage);
	}

	ParseNetstat(sqliteNet1, sqliteNet2);
	TerminateCurrentProcess(0, 1);
	return TRUE;
}

/*
	Description : terminate process found by PID.

	Parameter :
		( in )	DWORD32 targetPID : target terminated process' ID
		( in )	DWORD	mode
			0 -> Use PID
			1 -> Auto Kill
			else -> Error
	Return value :
		0 = Error
		1 = Success
*/
BOOL __stdcall TerminateCurrentProcess(CONST DWORD32 targetPID, CONST DWORD mode) {
	LPWSTR imageName = new WCHAR[MAX_PATH];
	DWORD imageNameSize = MAX_PATH;

	CPROINFO targetProcess;
	PCPROINFO pTargetProcess = &targetProcess;

	if ((targetProcess.hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0))
		== INVALID_HANDLE_VALUE) {
		return NULL;
	}
	targetProcess.proc32.dwSize = sizeof(PROCESSENTRY32);
	if (Process32First(targetProcess.hSnap, &targetProcess.proc32)) {
		while (Process32Next(targetProcess.hSnap, &targetProcess.proc32)) 
		{
			if (mode == 0) {
				if (targetProcess.proc32.th32ProcessID == targetPID) {
					DWORD exitCode = NULL;
					BOOL bInheritHandle = FALSE;
					DWORD result = 0;

					targetProcess.curHandle = OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, bInheritHandle, targetPID);
					if (targetProcess.curHandle == NULL) {
						return NULL;
					}
					_tprintf_s(L"Terminated Process, PID : %s, %d",
						targetProcess.proc32.szExeFile,
						targetProcess.proc32.th32ProcessID);
					if (TerminateProcess(targetProcess.curHandle, 0)) {
						_tprintf_s(L" -> Success\n");
						GetExitCodeProcess(targetProcess.curHandle, &exitCode);
					}
					else {
						_tprintf_s(L" -> Failed\n");
						CloseHandle(targetProcess.curHandle);
						return NULL;
					}
					CloseHandle(targetProcess.curHandle);
					return TRUE;
				}
			}
			else if (mode == 1) {
				if (targetProcess.proc32.th32ProcessID < 100) {

				}
				else {
					DWORD exitCode = NULL;
					BOOL bInheritHandle = FALSE;
					DWORD result = 0;

					targetProcess.curHandle = OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, bInheritHandle, targetProcess.proc32.th32ProcessID);
					if (targetProcess.curHandle == NULL) {
						continue;
					}
					result = QueryFullProcessImageName(targetProcess.curHandle, 0, imageName, &imageNameSize);
					imageNameSize = MAX_PATH;
					if (result == 0) {
						CloseHandle(targetProcess.curHandle);
						continue;
					}
					else {
						// Hard coding ( for the auto kill testing )
						result = wcscmp(imageName, L"C:\\Windows\\System32\\notepad.exe");
						if (result == 0) {
							_tprintf_s(L"Terminated Process, PID : %s, %d",
								targetProcess.proc32.szExeFile,
								targetProcess.proc32.th32ProcessID);
							if (TerminateProcess(targetProcess.curHandle, 0)) {
								_tprintf_s(L" -> Success\n");
								GetExitCodeProcess(targetProcess.curHandle, &exitCode);
							}
							else {
								_tprintf_s(L" -> Failed\n");
							}
						}
					}
					CloseHandle(targetProcess.curHandle);
				}
			}
			else {
				return NULL;
			}
		}
	}
	return TRUE;
}

/*
	Description : delete target registry value

	Parameter :
		( in )	TCHAR keyName[REGNAME_MAX] : deleted value's name
		( in )	DWORD32 targetKey  : 
			1 -> HKEY_CURRENT_USER/Microsoft/Windows/CurrentVersion/Run
			2 -> HKEY_LOCAL_MACHINE/Microsoft/Windows/CurrentVersion/Run
			3 -> HKEY_CURRENT_USER/Microsoft/Windows/CurrentVersion/RunOnce
			4 -> HKEY_LOCAL_MACHINE/Microsoft/Windows/CurrentVersion/RunOnce
			5 -> HKEY_LOCAL_MACHINE/SYSTEM/CurrentControlSet/Services
			else -> Error
	Return value :
		0 = Error
		1 = Success
*/
TWTL_SNAPSHOT_API BOOL __stdcall DeleteRunKey(TCHAR CONST keyName[REGNAME_MAX], CONST DWORD32 targetKey) {
	REGINFO reg;
	PREGINFO pReg = &reg;
	
	wcscpy_s(pReg->keyName, sizeof(pReg->keyName), keyName);
	
	DWORD32 target = targetKey;
	
	if (!OpenRegisteryKey(pReg, target)) {		
		if (!RegDeleteValue(pReg->key, pReg->keyName)) {
			_tprintf_s(L"Delete Value Name : %s\n", pReg->keyName);
		}
		else {
			_tprintf_s(L"Can't find value :(\n");
		}
	}
	else {
		return NULL;
	}
	_tprintf_s(L"\n");
	return TRUE;
}

/*
	Description : open register key

	Parameter :
		( in )	PREGINFO pReg : information of target entry
		( in )	DWORD target  :
			1 -> HKEY_CURRENT_USER/Microsoft/Windows/CurrentVersion/Run
			2 -> HKEY_LOCAL_MACHINE/Microsoft/Windows/CurrentVersion/Run
			3 -> HKEY_CURRENT_USER/Microsoft/Windows/CurrentVersion/RunOnce
			4 -> HKEY_LOCAL_MACHINE/Microsoft/Windows/CurrentVersion/RunOnce
			5 -> HKEY_LOCAL_MACHINE/SYSTEM/CurrentControlSet/Services
			else -> Error
	return value :
		0 = Success
		else = Error
*/
DWORD __stdcall OpenRegisteryKey(PREGINFO CONST pReg, CONST DWORD32 target) {
	HKEY mainEntry;

	if (target == 1 || target == 3) {
		mainEntry = HKEY_CURRENT_USER;
	}
	else if (target == 2 || target == 4 || target == 5) {
		mainEntry = HKEY_LOCAL_MACHINE;
	}
	else {
		return 1;
	}

	if (target == 1 || target == 2) {
		return	RegOpenKeyEx(mainEntry,
				L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
				NULL,
				KEY_ALL_ACCESS,
				&pReg->key);
	}
	else if (target == 3 || target == 4) {
		return	RegOpenKeyEx(mainEntry,
				L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce",
				NULL,
				KEY_ALL_ACCESS,
				&pReg->key);
	}
	else if (target == 5) {
		return  RegOpenKeyEx(mainEntry,
				L"SYSTEM\\CurrentControlSet\\Services",
				NULL,
				KEY_ALL_ACCESS,
				&pReg->key);
	}
	else {
		return 1;
	}
}

/*
	Description : set target registry entry

	Parameter :
		( out ) FILE* storage : written txt file
		( in )	PREGINFO pReg : information of target entry
		( in )	DWORD target  : 
			1 -> HKEY_CURRENT_USER/Microsoft/Windows/CurrentVersion/Run
			2 -> HKEY_LOCAL_MACHINE/Microsoft/Windows/CurrentVersion/Run
			3 -> HKEY_CURRENT_USER/Microsoft/Windows/CurrentVersion/RunOnce
			4 -> HKEY_LOCAL_MACHINE/Microsoft/Windows/CurrentVersion/RunOnce
			5 -> HKEY_LOCAL_MACHINE/SYSTEM/CurrentControlSet/Services
			else -> Error
		( in )	DWORD32 mode :
			0 -> just print
			1 -> txt export
			else -> error
	Return value :
		0 = Error
		1 = Success
*/
BOOL __stdcall SetTargetRegistryEntry(
	FILE* storage, 
	PREGINFO CONST pReg, 
	TWTL_DB_REGISTRY* sqliteReg, 
	TWTL_DB_SERVICE* sqliteSvc, 
	CONST DWORD32 target, 
	CONST DWORD32 mode) 
{
	if (!OpenRegisteryKey(pReg, target))
	{
		if (mode == 0) {
			if (!WriteRegToTxt(storage, pReg, sqliteReg, sqliteSvc, mode, target)) {
				return NULL;
			}
		}
		else if (mode == 1) {
			if (target == 1) {
				fwprintf_s(storage, L"<Register Run List ( for current users )>\n");
			}
			else if (target == 2) {
				fwprintf_s(storage, L"<Register Run List ( for all users )>\n");
			}
			else if (target == 3) {
				fwprintf_s(storage, L"<Register RunOnce List ( for current users )>\n");
			}
			else if (target == 4) {
				fwprintf_s(storage, L"<Register RunOnce List ( for all users )>\n");
			}
			else if (target == 5) {
				fwprintf_s(storage, L"<Service List>");
			}
			else {
				return NULL;
			}

			// call writing entry of target
			//
			if (!WriteRegToTxt(storage, pReg, sqliteReg, sqliteSvc, mode, target)) {
				return NULL;
			}

			if (target == 1) {
				fwprintf_s(storage, L"</Register Run List ( for current users )>\n\n");
			}
			else if (target == 2) {
				fwprintf_s(storage, L"</Register Run List ( for all users )>\n\n");
			}
			else if (target == 3) {
				fwprintf_s(storage, L"</Register RunOnce List ( for current users )>\n\n");
			}
			else if (target == 4) {
				fwprintf_s(storage, L"</Register RunOnce List ( for all users )>\n\n");
			}
			else if (target == 5) {
				fwprintf_s(storage, L"</Service List>");
			}
			else {
				return NULL;
			}
		}
		else {
			return NULL;
		}

		RegCloseKey(pReg->key);
	}
	else {
		fwprintf_s(storage, L"Can't get register key. :(\n");
		if (mode == 1) {
			fclose(storage);
		}
		return NULL;
	}

	return TRUE;
}

/*
	Description : Write register value in text file. ( txt export )

	Parameters : 
		( out ) FILE* storage : written by this function.
		( in )	PREGINFO pReg : information of target register entry.
		( in )	DWORD32 mode :
			0 -> just print
			1 -> txt export
			else -> error
		( in )	DWORD target  :
			1 -> HKEY_CURRENT_USER/Microsoft/Windows/CurrentVersion/Run
			2 -> HKEY_LOCAL_MACHINE/Microsoft/Windows/CurrentVersion/Run
			3 -> HKEY_CURRENT_USER/Microsoft/Windows/CurrentVersion/RunOnce
			4 -> HKEY_LOCAL_MACHINE/Microsoft/Windows/CurrentVersion/RunOnce
			5 -> HKEY_LOCAL_MACHINE/SYSTEM/CurrentControlSet/Services
			else -> Error
	Return value :
		0 = Error
		1 = Success
*/
BOOL __stdcall WriteRegToTxt(
	FILE* storage, 
	PREGINFO CONST pReg, 
	TWTL_DB_REGISTRY* sqliteReg, 
	TWTL_DB_SERVICE* sqliteSvc, 
	CONST DWORD32 mode, 
	CONST DWORD32 target)
{
	DWORD result = 0;

	if (target == 5) {
		TCHAR    achKey[REGNAME_MAX];		// buffer for subkey name
		DWORD    cbName;					// size of name string 
		TCHAR    achClass[MAX_PATH] = TEXT("");	// buffer for class name 
		DWORD    cchClassName = MAX_PATH;	// size of class string 
		DWORD    cSubKeys = 0;              // number of subkeys 
		DWORD    cbMaxSubKey;				// longest subkey size 
		DWORD    cchMaxClass;				// longest class string 
		DWORD    cValues;					// number of values for key 
		DWORD    cchMaxValue;				// longest value name 
		DWORD    cbMaxValueData;			// longest value data 
		DWORD    cbSecurityDescriptor;		// size of security descriptor 
		FILETIME ftLastWriteTime;			// last write time 

		DWORD i, retCode;

		TCHAR achValue[REGVALUE_MAX];
		DWORD cchValue = REGVALUE_MAX;

		// Get the class name and the value count. 

		retCode = RegQueryInfoKey(
			pReg->key,               // key handle 
			achClass,                // buffer for class name 
			&cchClassName,           // size of class string 
			NULL,                    // reserved 
			&cSubKeys,               // number of subkeys 
			&cbMaxSubKey,            // longest subkey size 
			&cchMaxClass,            // longest class string 
			&cValues,                // number of values for this key 
			&cchMaxValue,            // longest value name 
			&cbMaxValueData,         // longest value data 
			&cbSecurityDescriptor,   // security descriptor 
			&ftLastWriteTime);       // last write time 

		if (cSubKeys)
		{
			printf("\nNumber of subkeys: %d\n", cSubKeys);
			sqliteSvc = (TWTL_DB_SERVICE*)realloc(sqliteSvc, sizeof(TWTL_DB_SERVICE)*(cSubKeys + 10));
			
			HKEY HKLM = HKEY_LOCAL_MACHINE;
			REGINFO servReg;
			PREGINFO pServReg = &servReg;
			servReg.bufSize = REGNAME_MAX - 1;
			servReg.dataType = REG_SZ;

			for (i = 0; i < cSubKeys; i++)
			{
				cbName = REGNAME_MAX;
				retCode = RegEnumKeyEx(pReg->key, i,
					achKey,
					&cbName,
					NULL,
					NULL,
					NULL,
					&ftLastWriteTime);
				if (retCode == ERROR_SUCCESS)
				{
					wcscpy_s(sqliteSvc[i].key, REGNAME_MAX, achKey);
					_tprintf(TEXT("(%d) %s\n"), i + 1, sqliteSvc[i].key);
					
					if (cValues)
					{
						printf("\nNumber of values: %d\n", cValues);

						for (i = 0, retCode = ERROR_SUCCESS; i<cValues; i++)
						{
							cchValue = REGVALUE_MAX;
							achValue[0] = '\0';
							retCode = RegEnumValue(pReg->key, i,
								achValue,
								&cchValue,
								NULL,
								NULL,
								NULL,
								NULL);

							if (retCode == ERROR_SUCCESS)
							{
								_tprintf(TEXT("(%d) %s\n"), i + 1, achValue);
							}
						}
					}
				}
			}
		}
	}
	else {
		int regIndex = 0;
		for (int i = 0; result == ERROR_SUCCESS; i++) {
			result = RegEnumValue(pReg->key, i, pReg->keyName, &pReg->bufSize, NULL, NULL, NULL, NULL);
			if (result == ERROR_SUCCESS) {
				regIndex++;
			}
		}
		sqliteReg = (TWTL_DB_REGISTRY*)realloc(sqliteReg, sizeof(TWTL_DB_REGISTRY)*(regIndex + 2));
		for (int i = 0; result == ERROR_SUCCESS; i++)
		{
			if (1 <= target && target <= 4) {

				result = RegEnumValue(pReg->key, i, pReg->keyName, &pReg->bufSize, NULL, NULL, NULL, NULL);

				if (result == ERROR_SUCCESS)
				{
					if (1 <= target && target <= 4) {
						// must initialize reg.bufsize
						//
						if (!InitRegSize(pReg, 2)) {
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
						if (!InitRegSize(pReg, 1)) {
							return NULL;
						}
						if (mode == 0) {
							wcscpy_s(sqliteReg[i].value, REGVALUE_MAX, pReg->keyValue);
							wcscpy_s(sqliteReg[i].name, REGNAME_MAX, pReg->keyName);
							if (target == 1) {
								wcscpy_s(sqliteReg[i].path, 255, L"HKCU\\Microsoft\\Windows\\CurrentVersion\\Run");
							}
							else if (target == 2) {
								wcscpy_s(sqliteReg[i].path, 255, L"HKLM\\Microsoft\\Windows\\CurrentVersion\\Run");
							}
							else if (target == 3) {
								wcscpy_s(sqliteReg[i].path, 255, L"HKCU\\Microsoft\\Windows\\CurrentVersion\\RunOnce");
							}
							else if (target == 4) {
								wcscpy_s(sqliteReg[i].path, 255, L"HKLM\\Microsoft\\Windows\\CurrentVersion\\RunOnce");
							}
							sqliteReg[i].type = 1;

							_tprintf_s(L"Register Name : %s, Value : %s\n", sqliteReg[i].name, sqliteReg[i].value);
						}
						else if (mode == 1) {
							PrintCUI(pReg->keyName);
							PrintCUI(pReg->keyValue);
							fwprintf_s(storage, L"%s\t", pReg->keyName);
							fwprintf_s(storage, L"%s\n", pReg->keyValue);
						}
						else {
							return NULL;
						}
					}
					else {
						return NULL;
					}
				}
			}
			else {
				break;
			}
		}
	}
	return TRUE;
}