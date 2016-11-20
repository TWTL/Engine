// Main.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "JsonThread.h"

BOOL g_runJsonMainThread = FALSE;
BOOL g_runJsonTrapThread = FALSE;

int main()
{	
	HANDLE hJsonThread[2] = { INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE };

	DWORD select=NULL;
	DWORD32 targetKey = NULL;
	DWORD32 targetPID = NULL;
	TCHAR keyName[REGNAME_MAX] = { 0, };
	TCHAR keyValue[REGVALUE_MAX] = { 0, };

	SetPrivilege(SE_DEBUG_NAME, TRUE);

	hJsonThread[0] = (HANDLE)_beginthreadex(NULL, 0, &JsonMainThreadProc, (LPVOID)NULL, 0, NULL);
	hJsonThread[1] = (HANDLE)_beginthreadex(NULL, 0, &JsonTrapThreadProc, (LPVOID)NULL, 0, NULL);
	g_runJsonMainThread = TRUE;
	g_runJsonTrapThread = TRUE;
	wprintf_s(L"Json Threads running\n\n");

	while (TRUE) {
		_tprintf_s(L"1. current snapshot\n");
		_tprintf_s(L"2. current snapshot ( txt export )\n");
		_tprintf_s(L"3. \n");
		_tprintf_s(L"4. terminate process by PID\n");
		_tprintf_s(L"5. delete register key\n");
		_tprintf_s(L"6. \n");
		_tprintf_s(L"7. Inject DLL - Global\n");
		_tprintf_s(L"8. Eject DLL - Global\n");
		_tprintf_s(L"\n");
		_tprintf_s(L"Type Number : ");

		scanf_s("%d", &select);
		if (select == 1) {
			if (!SnapCurrentStatus(0)) {
				ErrMsg();
				DelayWait(5000);
			}
		}
		else if (select == 2) {
			if (!SnapCurrentStatus(1)) {
				ErrMsg();
				DelayWait(5000);
			}
		}
		else if (select == 3) {

		}
		else if (select == 4) {
			_tprintf_s(L"Type PID : ");
			wscanf_s(L"%d", &targetPID);
			TerminateCurrentProcess(targetPID);
		}
		else if (select == 5) {
			_tprintf_s(L"Type (keyName) (target): ");
			wscanf_s(L"%s%d", &keyName, REGNAME_MAX-1, &targetKey);
			if (!DeleteRunKey(keyName, targetKey)) {
				ErrMsg();
				DelayWait(5000);
			}
		}
		else if (select == 6) {

		}
		else if (select == 7){
			int nMode = INJECTION_MODE;
			InjectAllProcess(nMode, L"C:\\Windows\\System32\\TWTL_Snapshot.dll");

		}
		else if (select == 8) {
			int nMode = EJECTION_MODE;
			InjectAllProcess(nMode, L"C:\\Windows\\System32\\TWTL_Snapshot.dll");
		}
		else {
			break;
		}
		_tprintf_s(L"\n");
	}
	
	wprintf_s(L"\nTerminating Json Threads...\n");
	g_runJsonMainThread = FALSE;
	g_runJsonTrapThread = FALSE;
	WaitForMultipleObjects(2, hJsonThread, TRUE, INFINITE);
	CloseHandle(hJsonThread[0]);
	CloseHandle(hJsonThread[1]);

	wprintf_s(L"\nBye!\n");
	DelayWait(5000);
	
	return 0;
}

