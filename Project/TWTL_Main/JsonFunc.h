#include "stdafx.h"

#pragma once

#include "..\TWTL_Snapshot\MonitorFunc.h"

#define TWTL_PROTO_MAX_BUF 1024

typedef unsigned int (WINAPI *LPTHREADPROC)(LPVOID lpParam);

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

#define PROTO_STR_REQ_GET		"request.get"
#define PROTO_STR_REQ_SET		"request.set"
#define PROTO_STR_REQ_DIFF		"request.diff"
#define PROTO_STR_REQ_PATCH		"request.patch"
#define PROTO_STR_REQ_PUT		"request.put"
#define PROTO_STR_REQ_DELETE	"request.delete"
#define PROTO_STR_REQ_BETA		"request.beta"
#define PROTO_STR_RES_STATUS	"response.status"
#define PROTO_STR_RES_OBJECT	"response.object"
#define PROTO_STR_TRAP_CHANGE	"trap.change"
#define PROTO_STR_ACK_CHECK		"trap-ack.check"

enum twtl_status_code
{
	PROTO_STATUS_WAIT = 100,
	PROTO_STATUS_SUCCESS = 200,
	PROTO_STATUS_REDIRECT = 300,
	PROTO_STATUS_CLIENT_ERROR = 400,
	PROTO_STATUS_SERVER_ERROR = 500
};

#define INVALID_PORT_VALUE 0

enum twtl_proto_node_value
{
	PROTO_VALUE_STRING,
	PROTO_VALUE_INT32,
	PROTO_VALUE_UINT32,
	PROTO_VALUE_FLOAT32,
	PROTO_VALUE_BOOLEAN,
	PROTO_VALUE_PATCH_OBJECT,
	PROTO_VALUE_NULL
};

// "value":{"value":[{"Name":".NET CLR Data","Value":null}],"accept":[false],"reject":[true]}}]
// "value":{"ImagePath":""}
typedef struct twtL_proto_patch_node {
	std::string value_Name;
	std::string value_Value;
	BOOL accept;
	BOOL reject;
} TWTL_PROTO_PATCH_NODE;

typedef struct twtl_proto_node {
	int type;
	std::string path;
	int value_type;
	std::string value_string;
	int32_t value_int32;
	uint32_t value_uint32;
	double value_real;
	BOOL value_boolean;
	BOOL value_patch;
	TWTL_PROTO_PATCH_NODE value_patch_object;
	struct twtl_proto_node* next;
} TWTL_PROTO_NODE;

typedef struct twtl_proto_buf {
	std::string name; 
	std::string app; 
	std::string version;
	TWTL_PROTO_NODE* contents;
} TWTL_PROTO_BUF;

#ifndef TWTL_TRAP_H
#define TWTL_TRAP_H
#define TRAP_PATH_MAX 1024
typedef struct twtl_trap_queue_node {
	char path[TRAP_PATH_MAX];
	struct twtl_trap_queue_node* next;
} TWTL_TRAP_QUEUE_NODE;
typedef struct twtl_trap_queue {
	int count;
	struct twtl_trap_queue_node* node;
} TWTL_TRAP_QUEUE;
typedef BOOL(*JSON_EnqTrapQueue_t)(TWTL_TRAP_QUEUE* queue, char* inPath);
#endif


typedef struct twtl_info_engine_node {
	std::string name;		// const
	std::string app;		// const
	std::string version;	// const 
	SHORT reqPort;			// const : 5259
	SHORT trapPort;			// need to be set by client
} TWTL_INFO_ENGINE_NODE;

typedef struct twtl_info_data {
	TWTL_INFO_ENGINE_NODE engine;
} TWTL_INFO_DATA;

DWORD JSON_Parse(const char buf[], size_t buflen, TWTL_PROTO_BUF* req);

TWTL_PROTO_NODE* JSON_AddProtoNode(TWTL_PROTO_BUF* proto);
void JSON_ClearProtoNode(TWTL_PROTO_BUF* proto);

TWTL_TRAP_QUEUE* JSON_InitTrapQueue(TWTL_TRAP_QUEUE* queue);
BOOL JSON_EnqTrapQueue(TWTL_TRAP_QUEUE* queue, char* inPath);
BOOL JSON_DeqTrapQueue(TWTL_TRAP_QUEUE* queue, char* outPath);
void JSON_ClearTrapQueue(TWTL_TRAP_QUEUE* queue);

void JSON_ProtoParse(json_t *element, const char *key, TWTL_PROTO_BUF* req, TWTL_PROTO_NODE* node, int depth);

char* JSON_ProtoMakeResponse(TWTL_PROTO_BUF* req);
void JSON_ProtoReqGetProc(TWTL_PROTO_NODE* req_node, json_t* root);
void JSON_ProtoReqSetProc(TWTL_PROTO_NODE* req_node, json_t* root);
void JSON_ProtoReqDiffProc(TWTL_PROTO_NODE* req_node, json_t* root);
void JSON_ProtoReqPatchProc(TWTL_PROTO_NODE* req_node, json_t* root);
void JSON_ProtoReqBetaProc(TWTL_PROTO_NODE* req_node, json_t* root);

typedef enum twtl_reg_short_type
{
	REG_SHORT_HKCU_RUN,
	REG_SHORT_HKCU_RUNONCE,
	REG_SHORT_HKLM_RUN,
	REG_SHORT_HKLM_RUNONCE,
	REG_SHORT_SERVICE,
} TWTL_REG_SHORT_TYPE;

void JSON_DiffProc_RegShort(TWTL_PROTO_NODE* req_node, json_t* root, TWTL_REG_SHORT_TYPE type);
void JSON_PatchProc_RegShort(TWTL_PROTO_NODE* req_node, json_t* root, TWTL_REG_SHORT_TYPE type);

BOOL JSON_SendTrap(SOCKET sock, std::string path);

void JSON_Init_TWTL_INFO_DATA();
void JSON_Init_TWTL_INFO_ENGINE_NODE(TWTL_INFO_ENGINE_NODE* node);

void JSON_Init_ProtoBufHeader(TWTL_PROTO_BUF* buf);

