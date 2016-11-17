// TWTL_NetClient.cpp : �ܼ� ���� ���α׷��� ���� �������� �����մϴ�.
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
	SOCKET connectSocket;

	JSON_InitClientSocket(L"127.0.0.1", L"5259", &connectSocket);
	JSON_ProcClientSocket(&connectSocket);
	JSON_CloseClientSocket(&connectSocket);

    return 0;
}
