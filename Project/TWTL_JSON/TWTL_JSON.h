#include "stdafx.h"

#pragma once

#define TWTL_PROTO_MAX_BUF 1024

typedef unsigned int (WINAPI *LPTHREADPROC)(LPVOID lpParam);

enum twtl_proto_node_value { PROTO_VALUE_STR, PROTO_VALUE_INT, PROTO_VALUE_REAL, PROTO_VALUE_BOOL, PROTO_VALUE_NULL };
enum twtl_proto_type { PROTO_REQ_GET, PROTO_REQ_SET, PROTO_RES_STATUS, PROTO_RES_OBJECT };
enum twtl_status_code { PROTO_STATUS_WAIT = 100, PROTO_STATUS_SUCCESS = 200, PROTO_STATUS_REDIRECT = 300, PROTO_STATUS_CLIENT_ERROR = 400, PROTO_STATUS_SERVER_ERROR = 500 };
#define INVALID_PORT_VALUE 0


typedef struct twtl_proto_node {
	int type;
	char path[TWTL_PROTO_MAX_BUF];
	int value_type;
	char value_str[TWTL_PROTO_MAX_BUF];
	int64_t value_int;
	double value_real;
	bool value_bool;
	struct twtl_proto_node* next;
} TWTL_PROTO_NODE;

typedef struct twtl_proto_buf {
	char name[TWTL_PROTO_MAX_BUF];
	char app[TWTL_PROTO_MAX_BUF];
	char version[TWTL_PROTO_MAX_BUF];
	TWTL_PROTO_NODE* contents;
} TWTL_PROTO_BUF;

typedef struct twtl_info_engine_node {
	char name[TWTL_PROTO_MAX_BUF];		// const
	char version[TWTL_PROTO_MAX_BUF];	// const 
	SHORT reqPort;						// const : 5259
	SHORT trapPort;						// need to be set by client
} TWTL_INFO_ENGINE_NODE;

typedef struct twtl_info_data {
	TWTL_INFO_ENGINE_NODE engine;
} TWTL_INFO_DATA;

DWORD JSON_Parse(const char buf[], size_t buflen, TWTL_PROTO_BUF* req);
TWTL_PROTO_NODE* JSON_ProtoAddNode(TWTL_PROTO_BUF* proto);
void JSON_ProtoClearNode(TWTL_PROTO_BUF* proto);

void JSON_ProtoParse(json_t *element, const char *key, TWTL_PROTO_BUF* req, TWTL_PROTO_NODE* node, int depth);
void JSON_ProtoMakeResponse(TWTL_PROTO_BUF* req, TWTL_PROTO_BUF* res);
json_t* JSON_ProtoBufToJson(TWTL_PROTO_BUF* res);

TWTL_JSON_API void JSON_Init_TWTL_INFO_DATA();
void JSON_Init_TWTL_INFO_ENGINE_NODE(TWTL_INFO_ENGINE_NODE* node);
