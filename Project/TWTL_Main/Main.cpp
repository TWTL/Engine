// Main.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

int main()
{	
	int select=NULL;
	while (TRUE) {
		_tprintf_s(L"1. current snapshot\n");
		_tprintf_s(L"2. current snapshot ( txt export )\n\n");
		_tprintf_s(L"Type Number : ");

		scanf_s("%d", &select);
		if (select == 1) {
			if (!snapCurrentStatus(0)) {
				errMsg();
				delayWait(5000);
			}
		}
		else if (select == 2) {
			if (!snapCurrentStatus(1)) {
				errMsg();
				delayWait(5000);
			}
		}
		else {
			break;
		}
		_tprintf_s(L"\n");
	}
	
	_tprintf_s(L"\nBye!\n");
	delayWait(5000);
	
	return 0;
}

