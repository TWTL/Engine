// TWTL_NetClient.cpp : �ܼ� ���� ���α׷��� ���� �������� �����մϴ�.
//

#include "stdafx.h"

int main()
{
	BOOL temp = true;

	JSON_InitTrapSocket("127.0.0.1", "5259");
	JSON_ProcTrapSocket(&temp);
	JSON_CloseTrapSocket();

    return 0;
}
