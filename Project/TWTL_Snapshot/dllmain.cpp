// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "Misc.h"
#include "MonitorFunc.h"

#define STR_MODULE_NAME	L"TWTL_Snapshot.dll"
#define STATUS_SUCCESS	(0x00000000L)

typedef enum _SYSTEM_INFORMATION_CLASS {
	SystemBasicInformation = 0,
	SystemPerformanceInformation = 2,
	SystemTimeOfDayInformation = 3,
	SystemProcessInformation = 5,
	SystemProcessorPerformanceInformation = 8,
	SystemInterruptInformation = 23,
	SystemExceptionInformation = 33,
	SystemRegistryQuotaInformation = 37,
	SystemLookasideInformation = 45
} SYSTEM_INFORMATION_CLASS;

typedef struct _SYSTEM_PROCESS_INFORMATION {
	ULONG NextEntryOffset;
	BYTE Reserved1[52];
	PVOID Reserved2[3];
	HANDLE UniqueProcessId;
	PVOID Reserved3;
	ULONG HandleCount;
	BYTE Reserved4[4];
	PVOID Reserved5[11];
	SIZE_T PeakPagefileUsage;
	SIZE_T PrivatePageCount;
	LARGE_INTEGER Reserved6[6];
} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;

typedef BOOL(WINAPI *PFCREATEPROCESSA)(
	LPCTSTR lpApplicationName,
	LPTSTR lpCommandLine,
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPCTSTR lpCurrentDirectory,
	LPSTARTUPINFO lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation
	);

typedef BOOL(WINAPI *PFCREATEPROCESSW)(
	LPCTSTR lpApplicationName,
	LPTSTR lpCommandLine,
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPCTSTR lpCurrentDirectory,
	LPSTARTUPINFO lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation
	);

BYTE g_pOrgCPA[5] = { 0, };
BYTE g_pOrgCPW[5] = { 0, };

BOOL hook_by_code(LPCTSTR szDllName, LPCSTR szFuncName, PROC pfnNew, PBYTE pOrgBytes)
{
	FARPROC pFunc;
	DWORD dwOldProtect, dwAddress;
	BYTE pBuf[5] = { 0xE9, 0, };
	PBYTE pByte;

	pFunc = (FARPROC)GetProcAddress(GetModuleHandle(szDllName), szFuncName);
	pByte = (PBYTE)pFunc;
	if (pByte[0] == 0xE9)
		return FALSE;

	VirtualProtect((LPVOID)pFunc, 5, PAGE_EXECUTE_READWRITE, &dwOldProtect);

	memcpy(pOrgBytes, pFunc, 5);

	dwAddress = (DWORD)pfnNew - (DWORD)pFunc - 5;
	memcpy(&pBuf[1], &dwAddress, 4);

	memcpy(pFunc, pBuf, 5);

	VirtualProtect((LPVOID)pFunc, 5, dwOldProtect, &dwOldProtect);

	return TRUE;
}

BOOL unhook_by_code(LPCTSTR szDllName, LPCSTR szFuncName, PBYTE pOrgBytes)
{
	FARPROC pFunc;
	DWORD dwOldProtect;
	PBYTE pByte;

	pFunc = (FARPROC)GetProcAddress(GetModuleHandle(szDllName), szFuncName);
	pByte = (PBYTE)pFunc;
	if (pByte[0] != 0xE9)
		return FALSE;

	VirtualProtect((LPVOID)pFunc, 5, PAGE_EXECUTE_READWRITE, &dwOldProtect);

	memcpy(pFunc, pOrgBytes, 5);

	VirtualProtect((LPVOID)pFunc, 5, dwOldProtect, &dwOldProtect);

	return TRUE;
}

BOOL InjectDll2(HANDLE hProcess, LPCTSTR szDllName)
{
	HANDLE hThread;
	LPVOID pRemoteBuf;
	DWORD dwBufSize = lstrlen(szDllName) + 1;
	FARPROC pThreadProc;

	pRemoteBuf = VirtualAllocEx(hProcess, NULL, dwBufSize,
		MEM_COMMIT, PAGE_READWRITE);
	if (pRemoteBuf == NULL)
		return FALSE;

	WriteProcessMemory(hProcess, pRemoteBuf, (LPVOID)szDllName,
		dwBufSize, NULL);

	pThreadProc = GetProcAddress(GetModuleHandle(L"kernel32.dll"), "LoadLibraryA");
	hThread = CreateRemoteThread(hProcess, NULL, 0,
		(LPTHREAD_START_ROUTINE)pThreadProc,
		pRemoteBuf, 0, NULL);
	WaitForSingleObject(hThread, INFINITE);

	VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);

	CloseHandle(hThread);

	return TRUE;
}

BOOL WINAPI NewCreateProcessA(
	LPCTSTR lpApplicationName,
	LPTSTR lpCommandLine,
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPCTSTR lpCurrentDirectory,
	LPSTARTUPINFO lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation
)
{
	BOOL bRet;
	FARPROC pFunc;

	unhook_by_code(L"kernel32.dll", "CreateProcessA", g_pOrgCPA);

	pFunc = GetProcAddress(GetModuleHandle(L"kernel32.dll"), "CreateProcessA");
	bRet = ((PFCREATEPROCESSA)pFunc)(lpApplicationName,
		lpCommandLine,
		lpProcessAttributes,
		lpThreadAttributes,
		bInheritHandles,
		dwCreationFlags,
		lpEnvironment,
		lpCurrentDirectory,
		lpStartupInfo,
		lpProcessInformation);

	if (bRet) {
		InjectDll2(lpProcessInformation->hProcess, STR_MODULE_NAME);
	}

	hook_by_code(L"kernel32.dll", "CreateProcessA",
		(PROC)NewCreateProcessA, g_pOrgCPA);

	return bRet;
}

BOOL WINAPI NewCreateProcessW(
	LPCTSTR lpApplicationName,
	LPTSTR lpCommandLine,
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPCTSTR lpCurrentDirectory,
	LPSTARTUPINFO lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation
)
{
	BOOL bRet;
	FARPROC pFunc;

	unhook_by_code(L"kernel32.dll", "CreateProcessW", g_pOrgCPW);

	pFunc = GetProcAddress(GetModuleHandle(L"kernel32.dll"), "CreateProcessW");
	bRet = ((PFCREATEPROCESSW)pFunc)(lpApplicationName,
		lpCommandLine,
		lpProcessAttributes,
		lpThreadAttributes,
		bInheritHandles,
		dwCreationFlags,
		lpEnvironment,
		lpCurrentDirectory,
		lpStartupInfo,
		lpProcessInformation);

	if (bRet) {
		InjectDll2(lpProcessInformation->hProcess, STR_MODULE_NAME);
	}

	hook_by_code(L"kernel32.dll", "CreateProcessW",
		(PROC)NewCreateProcessW, g_pOrgCPW);

	return bRet;
}

BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	SetPrivilege(SE_DEBUG_NAME, TRUE);

	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		hook_by_code(L"kernel32.dll", "CreateProcessA",
			(PROC)NewCreateProcessA, g_pOrgCPA);
		hook_by_code(L"kernel32.dll", "CreateProcessW",
			(PROC)NewCreateProcessW, g_pOrgCPW);
		break;

	case DLL_PROCESS_DETACH:
		unhook_by_code(L"kernel32.dll", "CreateProcessA",
			g_pOrgCPA);
		unhook_by_code(L"kernel32.dll", "CreateProcessW",
			g_pOrgCPW);
		break;
	}

	return TRUE;
}