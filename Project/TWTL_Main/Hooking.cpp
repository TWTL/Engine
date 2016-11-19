#include "stdafx.h"
#include "Hooking.h"

BOOL __stdcall InjectDll(DWORD dwPID, LPCTSTR szDllPath)
{
	HANDLE hProcess, hThread;
	LPVOID pRemoteBuf;
	DWORD  dwBufSize = lstrlen(szDllPath) + 1;
	LPTHREAD_START_ROUTINE pThreadProc;
	LPCWSTR targetDLL = L"kernel32.dll";

	if (!(hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID)))
	{
		printf("OpenProcess(%d) failed!!!\n", dwPID);
		return FALSE;
	}

	pRemoteBuf = VirtualAllocEx(hProcess, NULL, dwBufSize,
		MEM_COMMIT, PAGE_READWRITE);

	WriteProcessMemory(hProcess, pRemoteBuf,
		(LPVOID)szDllPath, dwBufSize, NULL);

	pThreadProc = (LPTHREAD_START_ROUTINE)
				   GetProcAddress(GetModuleHandle(targetDLL),
				   "LoadLibraryA");
	hThread = CreateRemoteThread(hProcess, NULL, 0,
			  pThreadProc, pRemoteBuf, 0, NULL);

	WaitForSingleObject(hThread, INFINITE);

	VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
	
#ifdef _DEBUG
	_tprintf_s(L"Success : (%d)\n", dwPID);
#endif // DEBUG

	CloseHandle(hThread);
	CloseHandle(hProcess);

	return TRUE;
}

BOOL __stdcall EjectDll(DWORD dwPID, LPCTSTR szDllPath)
{
	BOOL bMore = FALSE, bFound = FALSE;
	HANDLE hSnapshot, hProcess, hThread;
	MODULEENTRY32 me = { sizeof(me) };
	LPTHREAD_START_ROUTINE pThreadProc;
	LPCWSTR targetDLL = L"kernel32.dll";

	if (INVALID_HANDLE_VALUE ==
		(hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPID)))
		return FALSE;

	bMore = Module32First(hSnapshot, &me);
	for (; bMore; bMore = Module32Next(hSnapshot, &me))
	{
		if (!_wcsicmp(me.szModule, szDllPath) ||
			!_wcsicmp(me.szExePath, szDllPath))
		{
			bFound = TRUE;
			break;
		}
	}

	if (!bFound)
	{
		CloseHandle(hSnapshot);
		return FALSE;
	}

	if (!(hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID)))
	{
		printf("OpenProcess(%d) failed!!!\n", dwPID);
		CloseHandle(hSnapshot);
		return FALSE;
	}

	pThreadProc = (LPTHREAD_START_ROUTINE)
				   GetProcAddress(GetModuleHandle(targetDLL),
				   "FreeLibrary");

	hThread = CreateRemoteThread(hProcess, NULL, 0,
			  pThreadProc, me.modBaseAddr, 0, NULL);

	WaitForSingleObject(hThread, INFINITE);

#ifdef _DEBUG
	_tprintf_s(L"Success : (%d)\n", dwPID);
#endif // DEBUG

	CloseHandle(hThread);
	CloseHandle(hProcess);
	CloseHandle(hSnapshot);

	return TRUE;
}

BOOL __stdcall InjectAllProcess(DWORD nMode, LPCTSTR szDllPath)
{
	DWORD dwPID = 0;
	HANDLE hSnapShot = INVALID_HANDLE_VALUE;
	PROCESSENTRY32 pe;

	pe.dwSize = sizeof(PROCESSENTRY32);
	hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);

	Process32First(hSnapShot, &pe);
	do
	{
		dwPID = pe.th32ProcessID;

		if (dwPID < 100)
			continue;

		if (nMode == INJECTION_MODE)
			InjectDll(dwPID, szDllPath);
		else
			EjectDll(dwPID, szDllPath);
	} while (Process32Next(hSnapShot, &pe));

	CloseHandle(hSnapShot);

	return TRUE;
}
