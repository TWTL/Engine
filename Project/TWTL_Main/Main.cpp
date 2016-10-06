// Main.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

int main()
{
	if (!snapCurrentStatus()) {
		errMsg();
		Sleep(5000);
		return 1;
	}
	return 0;
}

