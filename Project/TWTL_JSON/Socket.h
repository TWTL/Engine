#pragma once

#include "stdafx.h"

TWTL_JSON_API DWORD __stdcall JSON_InitServerSocket(WSADATA* wsaData, SOCKET* clientSocket);
TWTL_JSON_API DWORD __stdcall JSON_ProcServerSocket(SOCKET* clientSocket);
TWTL_JSON_API DWORD __stdcall JSON_CloseServerSocket(SOCKET* clientSocket);

TWTL_JSON_API DWORD __stdcall JSON_InitClientSocket(LPCWSTR address, LPCWSTR port, SOCKET* connectSocket);
TWTL_JSON_API DWORD __stdcall JSON_ProcClientSocket(SOCKET* connectSocket);
TWTL_JSON_API DWORD __stdcall JSON_CloseClientSocket(SOCKET* connectSocket);