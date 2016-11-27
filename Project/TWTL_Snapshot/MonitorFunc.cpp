// MonitorFunc.cpp : Functions of monitor.

#include "stdafx.h"
#include "MonitorFunc.h"
#pragma comment(lib, "Shlwapi.lib")

DWORD __stdcall OpenRegisteryKey(PREGINFO CONST pReg, CONST DWORD32 target);
BOOL __stdcall SetTargetRegistryEntry(FILE* storage, PREGINFO CONST pReg, TWTL_DB_REGISTRY* sqliteReg, TWTL_DB_SERVICE* sqliteSvc , DWORD structSize[], CONST DWORD32 target, CONST DWORD32 mode);
BOOL __stdcall WriteRegToTxt(FILE* storage, PREGINFO CONST pReg, TWTL_DB_REGISTRY* sqliteReg, TWTL_DB_SERVICE* sqliteSvc, DWORD structSize[], CONST DWORD32 mode, CONST DWORD32 target);
BOOL __stdcall RegDelnodeRecurse(HKEY hKeyRoot, LPTSTR lpSubKey);

/*
	Description : Write current process entry and register ( Run ) 
				  in text file. ( txt export )

	Parameters : 
		( out ) TWTL_DB_PROCESS* sqlitePrc : Result of parsing PROCESSENTRY32W
				Put NULL if you don't want to query
		( out ) TWTL_DB_REGISTRY* sqliteReg1 : Result of parsing HKCU\Software\windows\currentversion\run
				Put NULL if you don't want to query
		( out ) TWTL_DB_REGISTRY* sqliteReg2 : Result of parsing HKLM\Software\windows\currentversion\run
				Put NULL if you don't want to query
		( out ) TWTL_DB_REGISTRY* sqliteReg3 : Result of parsing HKCU\Software\windows\currentversion\runonce
				Put NULL if you don't want to query
		( out ) TWTL_DB_REGISTRY* sqliteReg4 : Result of parsing HKLM\Software\windows\currentversion\runonce
				Put NULL if you don't want to query
		( out ) TWTL_DB_REGISTRY* sqliteSvc : Result of parsing HKLM\System\currentcontrolset\services
				Put NULL if you don't want to query
		( out ) TWTL_DB_REGISTRY* sqliteReg1 : Result of parsing TCPTable
				Put NULL if you don't want to query
		( out ) TWTL_DB_REGISTRY* sqliteReg2 : Result of parsing UDPTable
				Put NULL if you don't want to query
		( out ) DWORD structSize : get size of reallocated memory.
		( in )	DWORD32 mode :
			0 -> print
			1 -> txt export ( deprecated )
			2 -> get size
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
	DWORD structSize[],
	CONST DWORD32 mode) 
{
	FILE* storage = NULL;

	CHAR fileName[MAX_PROC_NAME] = "Snapshot";
	TCHAR curPID[PROPID_MAX] = { 0, };

	REGINFO reg;
	PREGINFO pReg = &reg;
	reg.bufSize = REGNAME_MAX - 1;
	reg.dataType = REG_SZ;

	int i = 0;
	int listNum = 0;


	/*
		Deprecated!

	if (!MakeFileName(fileName)) {
		return NULL;
	}

	*/
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

	// Get Process Information
	if (sqlitePrc)
	{
		CPROINFO currentProcessInfo;
		PCPROINFO pCurrentProcessInfo = &currentProcessInfo;
		currentProcessInfo.pProc32 = &currentProcessInfo.proc32;

		WCHAR imageName[MAX_PATH];
		DWORD imageNameSize = MAX_PATH;

		if ((currentProcessInfo.hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0))
			== INVALID_HANDLE_VALUE) {
			ExceptionFileClose(storage, mode);
			return NULL;
		}

		currentProcessInfo.proc32.dwSize = sizeof(PROCESSENTRY32);
		DWORD32 PID = 0;
		if (Process32First(currentProcessInfo.hSnap, &currentProcessInfo.proc32)) {
			DWORD result = 0;
			i = 0;
			while ((Process32Next(currentProcessInfo.hSnap, &currentProcessInfo.proc32)))
			{
				if (currentProcessInfo.proc32.th32ProcessID == 4) {

				}
				else {
					listNum++;
				}
			}
			if (mode == 0 || mode == 1) {
				if (Process32First(currentProcessInfo.hSnap, &currentProcessInfo.proc32)) {
					result = 0;
					i = 0;

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
									sqlitePrc[i].time = time(0);
									wcscpy_s(sqlitePrc[i].process_path, MAX_PATH, imageName);
									wcscpy_s(sqlitePrc[i].process_name, DB_MAX_PROC_NAME, currentProcessInfo.proc32.szExeFile);
									sqlitePrc[i].pid = currentProcessInfo.proc32.th32ProcessID;
									sqlitePrc[i].ppid = currentProcessInfo.proc32.th32ParentProcessID;

									if (mode == 0) {
#ifdef _DEBUG
										_tprintf_s(L"%s ", sqlitePrc[i].process_path);
										_tprintf_s(L"Process name : %s, PID : %d, PPID :%d\n",
											currentProcessInfo.proc32.szExeFile,
											currentProcessInfo.proc32.th32ProcessID,
											currentProcessInfo.proc32.th32ParentProcessID);
#endif
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
			else if (mode == 2) {
				structSize[0] = listNum;
			}
		}
		CloseHandle(currentProcessInfo.hSnap);
	}
	
	

	// Write value of register key value ( Run of current user )
	//
	BOOL iResult = TRUE;
	if (sqliteReg1)
		iResult &= SetTargetRegistryEntry(storage, pReg, sqliteReg1, NULL, structSize, 1, mode);
	if (sqliteReg2)
		iResult &= SetTargetRegistryEntry(storage, pReg, sqliteReg2, NULL, structSize, 2, mode);
	if (sqliteReg3)
		iResult &= SetTargetRegistryEntry(storage, pReg, sqliteReg3, NULL, structSize, 3, mode);
	if (sqliteReg4)
		iResult &= SetTargetRegistryEntry(storage, pReg, sqliteReg4, NULL, structSize, 4, mode);
	if (sqliteSvc)
		iResult &= SetTargetRegistryEntry(storage, pReg, NULL, sqliteSvc, structSize, 5, mode);
	if (!iResult)
	{
		ExceptionFileClose(storage, mode);
		return NULL;
	}
	if (mode == 1) {
		fclose(storage);
	}

	if (sqliteNet1 && sqliteNet2)
		ParseNetstat(sqliteNet1, sqliteNet2, structSize, mode);

	return TRUE;
}

/*
	Description : terminate process found by PID or Matching Blacklist

	Parameter :
		( in )	DWORD32 targetPID : target terminated process' ID
			if mode is 1, it must be NULL.
		( in/out )TCHAR	imagePath : 
			if mode is 0, it'll be getting fullimagepath of target process.
			else if mode is 1, it must NULL.
		( in )TCHAR	imagePath : 
			if mode is 1, it must have list of blacklist processes' imagepath.
			else if mode is 0, it must NULL.
		( in )	DWORD	length : number of row of the imagepath array.
			if mode is 0, it must be NULL.
		( in )	DWORD	mode
			0 -> Use PID
			1 -> Auto Kill
			else -> Error
	Return value :
		0 = Error
		1 = Success
*/
TWTL_SNAPSHOT_API BOOL __stdcall TerminateCurrentProcess(CONST DWORD32 targetPID, TCHAR imagePath[], TCHAR(*blackList)[MAX_PATH], CONST DWORD length, CONST DWORD mode) {
	LPWSTR imageName = new WCHAR[MAX_PATH];
	DWORD imageNameSize = MAX_PATH;

	CPROINFO targetProcess;
	PCPROINFO pTargetProcess = &targetProcess;
	
	if (targetPID && mode == 1) {
		return NULL;
	}
	if (length && mode == 0) {
		return NULL;
	}

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
					result = QueryFullProcessImageName(targetProcess.curHandle, 0, imagePath, &imageNameSize);
					imageNameSize = MAX_PATH;

					if (result == 0) {
						CloseHandle(targetProcess.curHandle);
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
						for (DWORD i = 0; i < length; i++) {
							result = wcscmp(imageName, blackList[i]);
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
		( in )	TCHAR keyName[REGNAME_MAX] : deleted value's name or key's name
			in case of target 1 ~ 4 : value's name
			in case of target 5 : key's name
			else : must not executed
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
TWTL_SNAPSHOT_API BOOL __stdcall DeleteKeyOrKeyValue(TCHAR CONST keyName[REGNAME_MAX], CONST DWORD32 targetKey) {
	REGINFO reg;
	PREGINFO pReg = &reg;
	
	wcscpy_s(pReg->keyName, sizeof(pReg->keyName), keyName);
	
	DWORD32 target = targetKey;
	
	if (!OpenRegisteryKey(pReg, target)) {
		if (1 <= target && target <= 4) {

			if (!RegDeleteValue(pReg->key, pReg->keyName)) {
#ifdef _DEBUG
				_tprintf_s(L"Delete Value Name : %s\n", pReg->keyName);
#endif
			}
			else {
#ifdef _DEBUG
				_tprintf_s(L"Can't find value :(\n");
#endif
			}
		}
		else if (target == 5) {
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
					if (retCode == ERROR_SUCCESS) {
						if (!wcscmp(keyName, achKey)) {
							TCHAR szDelKey[MAX_PATH * 2]= L"SYSTEM\\CurrentControlSet\\Services\\";

							wcscat_s(szDelKey, MAX_PATH * 2, achKey);
							return RegDelnodeRecurse(HKEY_LOCAL_MACHINE, szDelKey);
						}
					}
				}
			}
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
			1 -> txt export ( deprecated )
			2 -> get size
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
	DWORD structSize[],
	CONST DWORD32 target, 
	CONST DWORD32 mode) 
{
	if (!OpenRegisteryKey(pReg, target))
	{
		if (mode == 0 || mode == 2) {
			if (!WriteRegToTxt(storage, pReg, sqliteReg, sqliteSvc, structSize, mode, target)) {
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
			if (!WriteRegToTxt(storage, pReg, sqliteReg, sqliteSvc, structSize, mode, target)) {
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
		( out ) DWORD structSize : get size of reallocated memory.
		( in )	DWORD32 mode :
			0 -> just print
			1 -> txt export ( deprecated )
			2 -> get size
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
	DWORD structSize[],
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
#ifdef _DEBUG
			printf("\nNumber of subkeys: %d\n", cSubKeys);
#endif 
			if (mode == 2) {
				structSize[5] = cSubKeys;
			}
			HKEY HKLM = HKEY_LOCAL_MACHINE;
			REGINFO servReg;
			PREGINFO pServReg = &servReg;
			servReg.bufSize = REGNAME_MAX - 1;
			servReg.dataType = REG_SZ;

			for (i = 0; i < cSubKeys; i++)
			{
				if (mode == 2) {
					break;
				}

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
					sqliteSvc[i].time = time(0);
					wcscpy_s(sqliteSvc[i].key, REGNAME_MAX, achKey);
#ifdef _DEBUG
					_tprintf(TEXT("(%d) %s"), i + 1, sqliteSvc[i].key);
#endif

					//34 + 255
					WCHAR path[289] = { 0, };
					wcscpy_s(path, REGNAME_MAX, L"SYSTEM\\CurrentControlSet\\Services\\");
					wcscat_s(path, REGNAME_MAX, sqliteSvc[i].key);
					REGINFO srvReg;
					PREGINFO pSrvReg = &srvReg;
					srvReg.bufSize = REGNAME_MAX - 1;
					srvReg.dataType = REG_SZ;
					if (!RegOpenKeyEx(HKLM,
						path,
						NULL,
						KEY_ALL_ACCESS,
						&pSrvReg->key))
					{
						result = 0;
						for (int i = 0; result == ERROR_SUCCESS; i++) {
							result = RegEnumValue(pSrvReg->key, i, pSrvReg->keyName, &pSrvReg->bufSize, NULL, NULL, NULL, NULL);
							if (result == ERROR_SUCCESS) {
								// must initialize reg.bufsize
								//
								if (!InitRegSize(pSrvReg, 2)) {
									return NULL;
								}

								if (RegQueryValueEx(pSrvReg->key,
									pSrvReg->keyName,
									NULL,
									&pSrvReg->dataType,
									(LPBYTE)pSrvReg->keyValue,
									&pSrvReg->bufSize)
									!= 0)
								{
								
								}
								else {
									if (!InitRegSize(pSrvReg, 1)) {
										return NULL;
									}
									if (!wcscmp(pSrvReg->keyName, L"ImagePath")) {
										TCHAR buffer[MAX_PATH] = { 0, };
										TCHAR buffer2[MAX_PATH] = { 0, };
										TCHAR *trash = NULL;
										wcscpy_s(buffer, MAX_PATH, pSrvReg->keyValue);
										PathRemoveArgs(buffer);
										ExpandEnvironmentStrings(buffer, buffer2, MAX_PATH);
										wcscpy_s(sqliteSvc[i].image_path, 255, buffer2);
#ifdef _DEBUG
										_tprintf_s(L" %s\n", sqliteSvc[i].image_path);
#endif
									}
								}
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
		if (mode == 2) {
			if (target == 1) {
				structSize[1] = regIndex;
			}
			else if (target == 2) {
				structSize[2] = regIndex;
			}
			else if (target == 3) {
				structSize[3] = regIndex;
			}
			else if (target == 4) {
				structSize[4] = regIndex;
			}
		}
		for (int i = 0; result == ERROR_SUCCESS; i++)
		{
			if (mode == 2) {
				break;
			}
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
							TCHAR buffer[MAX_PATH] = { 0, };
							TCHAR *trash = NULL;
							sqliteReg[i].time = time(0);
							wcscpy_s(buffer, MAX_PATH, pReg->keyValue);
							wcstok_s(buffer, L" ", &trash);

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
#ifdef _DEBUG
							_tprintf_s(L"Register Name : %s, Value : %s\n", sqliteReg[i].name, sqliteReg[i].value);
#endif
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


BOOL __stdcall RegDelnodeRecurse(HKEY hKeyRoot, LPTSTR lpSubKey)
{
	LPTSTR lpEnd;
	LONG lResult;
	DWORD dwSize;
	TCHAR szName[MAX_PATH];
	HKEY hKey;
	FILETIME ftWrite;

	// First, see if we can delete the key without having
	// to recurse.

	lResult = RegDeleteKey(hKeyRoot, lpSubKey);

	if (lResult == ERROR_SUCCESS)
		return TRUE;

	lResult = RegOpenKeyEx(hKeyRoot, lpSubKey, 0, KEY_READ, &hKey);

	if (lResult != ERROR_SUCCESS)
	{
		if (lResult == ERROR_FILE_NOT_FOUND) {
			printf("Key not found.\n");
			return TRUE;
		}
		else {
			printf("Error opening key.\n");
			return FALSE;
		}
	}

	// Check for an ending slash and add one if it is missing.

	lpEnd = lpSubKey + lstrlen(lpSubKey);

	if (*(lpEnd - 1) != TEXT('\\'))
	{
		*lpEnd = TEXT('\\');
		lpEnd++;
		*lpEnd = TEXT('\0');
	}

	// Enumerate the keys

	dwSize = MAX_PATH;
	lResult = RegEnumKeyEx(hKey, 0, szName, &dwSize, NULL,
		NULL, NULL, &ftWrite);

	if (lResult == ERROR_SUCCESS)
	{
		do {

			StringCchCopy(lpEnd, MAX_PATH * 2, szName);

			if (!RegDelnodeRecurse(hKeyRoot, lpSubKey)) {
				break;
			}

			dwSize = MAX_PATH;

			lResult = RegEnumKeyEx(hKey, 0, szName, &dwSize, NULL,
				NULL, NULL, &ftWrite);

		} while (lResult == ERROR_SUCCESS);
	}

	lpEnd--;
	*lpEnd = TEXT('\0');

	RegCloseKey(hKey);

	// Try again to delete the key.

	lResult = RegDeleteKey(hKeyRoot, lpSubKey);

	if (lResult == ERROR_SUCCESS)
		return TRUE;

	return FALSE;
}