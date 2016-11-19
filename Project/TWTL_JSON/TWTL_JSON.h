#include "stdafx.h"

#pragma once

#define TWTL_PROTO_MAX_BUF 1024

enum twtl_proto_node_value { PROTO_VALUE_STR, PROTO_VALUE_INT, PROTO_VALUE_REAL, PROTO_VALUE_BOOL, PROTO_VALUE_NULL };
enum twtl_proto_type { PROTO_REQ_GET, PROTO_REQ_SET, PROTO_RES_STATUS, PROTO_RES_OBJECT };

typedef struct twtl_proto_node {
	int type;
	char name[TWTL_PROTO_MAX_BUF];
	int value_type;
	char value_str[TWTL_PROTO_MAX_BUF];
	int value_int;
	double value_real;
	bool value_bool;
	TWTL_PROTO_NODE* next;
} TWTL_PROTO_NODE;

typedef struct twtl_protocol {
	char name[TWTL_PROTO_MAX_BUF];
	char app[TWTL_PROTO_MAX_BUF];
	char version[TWTL_PROTO_MAX_BUF];
	int type;
	TWTL_PROTO_NODE* contents;
} TWTL_PROTO_BUF;

typedef struct twtl_server_data {
	char name[];
} TWTL_SERVER_DATA;

DWORD JSON_Parse(const char buf[], size_t buflen);
TWTL_PROTO_NODE* JSON_ProtoAddNode(TWTL_PROTO_BUF* proto);
void JSON_ProtoClearNode(TWTL_PROTO_BUF* proto);
