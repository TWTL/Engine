// TWTL_NetClient.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#ifdef _DEBUG
#pragma comment(lib, "..\\Debug\\TWTL_JSON.lib")
#else
#pragma comment(lib, "..\\Release\\TWTL_JSON.lib")
#endif

#include "..\TWTL_JSON\TWTL_JSON.h"
#include "..\TWTL_JSON\Socket.h"

int main()
{
	BOOL temp = true;

	JSON_InitTrapSocket("127.0.0.1", "5259");
	JSON_ProcTrapSocket(&temp);
	JSON_CloseTrapSocket();

    return 0;
}
