#include "stdafx.h"

#pragma once

#define TWTL_PROTO_MAX_BUF 1024

typedef unsigned int (WINAPI *LPTHREADPROC)(LPVOID lpParam);

enum twtl_proto_node_value
{
	PROTO_VALUE_STRING,
	PROTO_VALUE_INT32,
	PROTO_VALUE_UINT32,
	PROTO_VALUE_FLOAT32,
	PROTO_VALUE_BOOLEAN,
	PROTO_VALUE_NULL
};

enum twtl_proto_type
{ 
	PROTO_REQ_GET, 
	PROTO_REQ_SET, 
	PROTO_REQ_DIFF,
	PROTO_REQ_PATCH,
	PROTO_REQ_PUT,
	PROTO_REQ_DELETE,
	PROTO_REQ_BETA,
	PROTO_RES_STATUS, 
	PROTO_RES_OBJECT,
	PROTO_TRAP_CHANGE,
	PROTO_TRAP_ACK_CHECK
};

enum twtl_status_code
{
	PROTO_STATUS_WAIT = 100,
	PROTO_STATUS_SUCCESS = 200,
	PROTO_STATUS_REDIRECT = 300,
	PROTO_STATUS_CLIENT_ERROR = 400,
	PROTO_STATUS_SERVER_ERROR = 500
};

#define INVALID_PORT_VALUE 0


typedef struct twtl_proto_node {
	int type;
	std::string path;
	int value_type;
	std::string value_string;
	int32_t value_int32;
	uint32_t value_uint32;
	float value_float32;
	bool value_boolean;
	struct twtl_proto_node* next;
} TWTL_PROTO_NODE;

typedef struct twtl_proto_buf {
	std::string name; 
	std::string app; 
	std::string version;
	TWTL_PROTO_NODE* contents;
} TWTL_PROTO_BUF;

typedef struct twtl_trap_queue_node {
	TWTL_PROTO_BUF buf;
	struct twtl_trap_queue_node* next;
} TWTL_TRAP_QUEUE_NODE;

typedef struct twtl_trap_queue {
	int count;
	struct twtl_trap_queue_node* node;
} TWTL_TRAP_QUEUE;

typedef struct twtl_info_engine_node {
	std::string name;		// const
	std::string app;		// const
	std::string version;	// const 
	SHORT reqPort;						// const : 5259
	SHORT trapPort;						// need to be set by client
} TWTL_INFO_ENGINE_NODE;

typedef struct twtl_info_data {
	TWTL_INFO_ENGINE_NODE engine;
} TWTL_INFO_DATA;

DWORD JSON_Parse(const char buf[], size_t buflen, TWTL_PROTO_BUF* req);

TWTL_PROTO_NODE* JSON_AddProtoNode(TWTL_PROTO_BUF* proto);
void JSON_ClearProtoNode(TWTL_PROTO_BUF* proto);

TWTL_TRAP_QUEUE* JSON_InitTrapQueue(TWTL_TRAP_QUEUE* queue);
BOOL JSON_EnqTrapQueue(TWTL_TRAP_QUEUE* queue, TWTL_PROTO_BUF* inBuf);
BOOL JSON_DeqTrapQueue(TWTL_TRAP_QUEUE* queue, TWTL_PROTO_BUF* outBuf);
void JSON_ClearTrapQueue(TWTL_TRAP_QUEUE* queue);

void JSON_ProtoParse(json_t *element, const char *key, TWTL_PROTO_BUF* req, TWTL_PROTO_NODE* node, int depth);
void JSON_ProtoMakeResponse(TWTL_PROTO_BUF* req, TWTL_PROTO_BUF* res);
json_t* JSON_ProtoBufToJson(TWTL_PROTO_BUF* res);

void JSON_Init_TWTL_INFO_DATA();
void JSON_Init_TWTL_INFO_ENGINE_NODE(TWTL_INFO_ENGINE_NODE* node);

void JSON_Init_ProtoBufHeader(TWTL_PROTO_BUF* buf);