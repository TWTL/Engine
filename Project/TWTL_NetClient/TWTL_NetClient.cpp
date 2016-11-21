// TWTL_NetClient.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
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
