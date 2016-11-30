#pragma once

#include "stdafx.h"

#include "MonitorFunc.h"

#define addr_size (3 + 3*4 + 1)   // xxx.xxx.xxx.xxx\0

#ifndef TWTL_TRAP_H
#define TWTL_TRAP_H
#define TRAP_MAX_PATH 1024
typedef struct twtl_trap_queue_node {
	char path[TRAP_MAX_PATH];
	struct twtl_trap_queue_node* next;
} TWTL_TRAP_QUEUE_NODE;
typedef struct twtl_trap_queue {
	int count;
	struct twtl_trap_queue_node* node;
} TWTL_TRAP_QUEUE;
typedef BOOL(*JSON_EnqTrapQueue_t)(TWTL_TRAP_QUEUE* queue, char* inPath);
#endif

BOOL
__stdcall
ParseNetstat(
	TWTL_DB_NETWORK* sqliteNet1,
	TWTL_DB_NETWORK* sqliteNet2,
	DWORD structSize[],
	JSON_EnqTrapQueue_t trapProc,
	TWTL_TRAP_QUEUE* queue,
	sqlite3* db,
	CONST DWORD32 mode
);