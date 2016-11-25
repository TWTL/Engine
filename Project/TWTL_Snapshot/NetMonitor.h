#pragma once

#include "stdafx.h"

#define LOCALADDRESS 0x0100007f

//
// Maximum string lengths for ASCII ip address and port names
//
#define HOSTNAMELEN		256
#define PORTNAMELEN		256
#define ADDRESSLEN		HOSTNAMELEN+PORTNAMELEN

//
// Our option flags
//
#define FLAG_ALL_ENDPOINTS	1
#define FLAG_SHOW_NUMBERS	2


//
// Undocumented extended information structures available 
// only on XP and higher
// 


typedef struct {
	DWORD   dwState;        // state of the connection
	DWORD   dwLocalAddr;    // address on local computer
	DWORD   dwLocalPort;    // port number on local computer
	DWORD   dwRemoteAddr;   // address on remote computer
	DWORD   dwRemotePort;   // port number on remote computer
	DWORD	  dwProcessId;
} MIB_TCPEXROW, *PMIB_TCPEXROW;


typedef struct {
	DWORD			dwNumEntries;
	MIB_TCPEXROW	table[ANY_SIZE];
} MIB_TCPEXTABLE, *PMIB_TCPEXTABLE;



typedef struct {
	DWORD   dwLocalAddr;    // address on local computer
	DWORD   dwLocalPort;    // port number on local computer
	DWORD	  dwProcessId;
} MIB_UDPEXROW, *PMIB_UDPEXROW;


typedef struct {
	DWORD			dwNumEntries;
	MIB_UDPEXROW	table[ANY_SIZE];
} MIB_UDPEXTABLE, *PMIB_UDPEXTABLE;

#define addr_size (3 + 3*4 + 1)   // xxx.xxx.xxx.xxx\0

BOOL
__stdcall
ParseNetstat(
);