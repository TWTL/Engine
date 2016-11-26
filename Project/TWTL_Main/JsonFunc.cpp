// TWTL_JSON.cpp : DLL 응용 프로그램을 위해 내보낸 함수를 정의합니다.
//

#include "stdafx.h"

#include "JsonFunc.h"

/*
* Parse text into a JSON object. If text is valid JSON, returns a
* json_t structure, otherwise prints and error and returns null.
*/
#define MAX_CHARS 4096

static TWTL_INFO_DATA g_twtlInfo;
static HANDLE g_hMutex;
extern TWTL_TRAP_QUEUE trapQueue;
extern SHORT trapPort;

// Must call JSON_ClearNode outside
DWORD JSON_Parse(const char buf[], size_t buflen, TWTL_PROTO_BUF* req)
{
	/* parse text into JSON structure */
	json_t *root;
	json_error_t error;
	memset(req, 0, sizeof(TWTL_PROTO_BUF));

	printf("[Recv]\n%s\n", buf);
	BinaryDump((uint8_t*)buf, buflen);

	root = json_loads(buf, 0, &error);
	// root = json_loadb(buf, buflen, 0, &error); // Why this function explodes?

	if (!root) { // Not valid JSON text
		fprintf(stderr, "json error on line %d: %s\n", error.line, error.text);
		return TRUE;
	}

	JSON_ProtoParse(root, "root", req, NULL, 0);
	json_decref(root);

	return FALSE;
}

TWTL_PROTO_NODE* JSON_AddProtoNode(TWTL_PROTO_BUF* req)
{
	TWTL_PROTO_NODE** now = &req->contents;
	while (*now != NULL)
	{
		now = (&(*now)->next);
	}

	(*now) = (TWTL_PROTO_NODE*) malloc(sizeof(TWTL_PROTO_NODE));

	memset((*now), 0, sizeof(TWTL_PROTO_NODE));

	return (*now);
}

void JSON_ClearProtoNode(TWTL_PROTO_BUF* buf)
{
	if (buf)
	{
		TWTL_PROTO_NODE* now = buf->contents;
		while (now != NULL)
		{
			TWTL_PROTO_NODE* next = now->next;
			if (now != NULL)
				free(now);
			now = next;
		}
		memset(buf, 0, sizeof(TWTL_PROTO_BUF));
	}
	

	return;
}

TWTL_TRAP_QUEUE* JSON_InitTrapQueue(TWTL_TRAP_QUEUE* queue)
{
	memset(queue, 0, sizeof(TWTL_TRAP_QUEUE));
	queue->count = 0;
	queue->node = NULL;

	g_hMutex = CreateMutex(
		NULL,              // default security attributes
		FALSE,             // initially not owned
		NULL);             // unnamed mutex

	if (!g_hMutex)
	{
		fprintf(stderr, "CreateMutex error: %lu\n", GetLastError());
		return NULL;
	}

	return queue;
}

BOOL JSON_EnqTrapQueue(TWTL_TRAP_QUEUE* queue, TWTL_PROTO_BUF* inBuf)
{
	DWORD dwWaitResult = WaitForSingleObject(g_hMutex, INFINITE);

	switch (dwWaitResult)
	{
		// The thread got ownership of the mutex
	case WAIT_OBJECT_0:
		__try {
			// TODO: Write to the database
			TWTL_TRAP_QUEUE_NODE** node = &(queue->node);
			while (*node != NULL)
				node = &((*node)->next);

			(*node) = (TWTL_TRAP_QUEUE_NODE*)malloc(sizeof(TWTL_TRAP_QUEUE_NODE));
			memcpy(&(*node)->buf, inBuf, sizeof(TWTL_PROTO_BUF));
			(*node)->next = NULL;
			queue->count++;
		}
		__finally {
			// Release ownership of the mutex object
			if (!ReleaseMutex(g_hMutex))
			{
				// Handle error.
			}
		}
		break;

		// The thread got ownership of an abandoned mutex
		// The database is in an indeterminate state
	case WAIT_ABANDONED:
		return TRUE;
	}

	return FALSE;
}

BOOL JSON_DeqTrapQueue(TWTL_TRAP_QUEUE* queue, TWTL_PROTO_BUF* outBuf)
{
	DWORD dwWaitResult = WaitForSingleObject(g_hMutex, INFINITE);

	switch (dwWaitResult)
	{
		// The thread got ownership of the mutex
	case WAIT_OBJECT_0:
		__try {
			// TODO: Write to the database
			if (0 < queue->count)
			{
				TWTL_TRAP_QUEUE_NODE* next = queue->node->next;
				memcpy(outBuf, &(queue->node->buf), sizeof(TWTL_PROTO_BUF));
				queue->node = queue->node->next;
				free(next);
				queue->count--;
				return FALSE;
			}
			else
				return TRUE;
		}
		__finally {
			// Release ownership of the mutex object
			if (!ReleaseMutex(g_hMutex))
			{
				// Handle error.
			}
		}
		break;

		// The thread got ownership of an abandoned mutex
		// The database is in an indeterminate state
	case WAIT_ABANDONED:
		return TRUE;
	}

	return FALSE;
}


void JSON_ClearTrapQueue(TWTL_TRAP_QUEUE* queue)
{
	if (0 < queue->count)
	{
		TWTL_TRAP_QUEUE_NODE* next = queue->node->next;
		queue->node = queue->node->next;
		free(next);
		queue->count--;
	}

	CloseHandle(g_hMutex);
}

void JSON_ProtoParse(json_t *element, const char *key, TWTL_PROTO_BUF* req, TWTL_PROTO_NODE* node, int depth)
{
	size_t i;
	size_t size;
	const char *foundKey;
	json_t *value;

	if (depth < 2)
	{
		switch (json_typeof(element)) {
		case JSON_OBJECT:
			size = json_object_size(element);
			json_object_foreach(element, foundKey, value) {
				JSON_ProtoParse(value, foundKey, req, node, depth + 1);
			}
			break;
		case JSON_ARRAY:
			size = json_array_size(element);
			json_array_foreach(element, i, value) {
				value = json_array_get(element, i);
				if (StrCmpIA(key, "contents") == 0)
				{
					TWTL_PROTO_NODE* new_node = JSON_AddProtoNode(req);
					JSON_ProtoParse(value, "array", req, new_node, depth + 1);
				}
				else
				{
					JSON_ProtoParse(value, "array", req, node, depth + 1);
				}
			}
			break;
		case JSON_STRING:
			if (StrCmpIA(key, "name") == 0)
			{
				req->name = json_string_value(element);
			}
			else if (StrCmpIA(key, "app") == 0)
			{
				req->app = json_string_value(element);
			}
			else if (StrCmpIA(key, "version") == 0)
			{
				req->version = json_string_value(element);
			}
			break;
		case JSON_INTEGER:
		case JSON_REAL:
		case JSON_TRUE:
		case JSON_FALSE:
		case JSON_NULL:
			break;
		default:
			fprintf(stderr, "unrecognized JSON type %d\n", json_typeof(element));
		}
	}
	else if (2 <= depth)
	{
		switch (json_typeof(element)) {
		case JSON_OBJECT:
			size = json_object_size(element);
			json_object_foreach(element, foundKey, value) {
				// TWTL_PROTO_NODE* new_node = JSON_ProtoAddNode(req);
				// JSON_ProtoParse(value, foundKey, req, new_node, depth + 1);
				JSON_ProtoParse(value, foundKey, req, node, depth + 1);
			}
			break;
		case JSON_ARRAY:
			size = json_array_size(element);

			printf("JSON Array of %ld element\n", size);
			for (i = 0; i < size; i++) {
				value = json_array_get(element, i);
				JSON_ProtoParse(value, "array", req, node, depth + 1);
			}
			break;
		case JSON_STRING:
			if (StrCmpIA(key, "type") == 0)
			{
				if (StrCmpIA(json_string_value(element), PROTO_STR_REQ_GET) == 0)
					node->type = PROTO_REQ_GET;
				else if (StrCmpIA(json_string_value(element), PROTO_STR_REQ_SET) == 0)
					node->type = PROTO_REQ_SET;
				else if (StrCmpIA(json_string_value(element), PROTO_STR_REQ_DIFF) == 0)
					node->type = PROTO_REQ_DIFF;
				else if (StrCmpIA(json_string_value(element), PROTO_STR_REQ_PATCH) == 0)
					node->type = PROTO_REQ_PATCH;
				else if (StrCmpIA(json_string_value(element), PROTO_STR_REQ_PUT) == 0)
					node->type = PROTO_REQ_PUT;
				else if (StrCmpIA(json_string_value(element), PROTO_STR_REQ_DELETE) == 0)
					node->type = PROTO_REQ_DELETE;
				else if (StrCmpIA(json_string_value(element), PROTO_STR_REQ_BETA) == 0)
					node->type = PROTO_REQ_BETA;
				else if (StrCmpIA(json_string_value(element), PROTO_STR_RES_STATUS) == 0)
					node->type = PROTO_RES_STATUS;
				else if (StrCmpIA(json_string_value(element), PROTO_STR_RES_OBJECT) == 0)
					node->type = PROTO_RES_OBJECT;
				else if (StrCmpIA(json_string_value(element), PROTO_STR_TRAP_CHANGE) == 0)
					node->type = PROTO_TRAP_CHANGE;
				else if (StrCmpIA(json_string_value(element), PROTO_STR_ACK_CHECK) == 0)
					node->type = PROTO_TRAP_ACK_CHECK;
			}
			else if (StrCmpIA(key, "path") == 0)
			{
				node->path = json_string_value(element);
			}
			else if (StrCmpIA(key, "value") == 0)
			{
				node->value_type = PROTO_VALUE_STRING;
				node->value_string = json_string_value(element);
			}
			break;
		case JSON_INTEGER:
			if (StrCmpIA(key, "value") == 0)
			{
				node->value_type = PROTO_VALUE_INT32;
				node->value_int32 = (int32_t) json_integer_value(element);
			}
			break;
		case JSON_REAL:
			if (StrCmpIA(key, "value") == 0)
			{
				node->value_type = PROTO_VALUE_FLOAT32;
				node->value_real = json_real_value(element);
			}
			break;
		case JSON_TRUE:
			if (StrCmpIA(key, "value") == 0)
			{
				node->value_type = PROTO_VALUE_BOOLEAN;
				node->value_boolean = TRUE;
			}
			break;
		case JSON_FALSE:
			if (StrCmpIA(key, "value") == 0)
			{
				node->value_type = PROTO_VALUE_BOOLEAN;
				node->value_boolean = FALSE;
			}
			break;
		case JSON_NULL:
			if (StrCmpIA(key, "value") == 0)
			{
				node->value_type = PROTO_VALUE_NULL;
			}
			break;
		default:
			fprintf(stderr, "unrecognized JSON type %d\n", json_typeof(element));
		}
	}	
}

char* JSON_ProtoMakeResponse(TWTL_PROTO_BUF* req)
{
	TWTL_PROTO_NODE* req_node = req->contents;
	char* res;

	json_t* root = json_object();

	json_object_set(root, "name", json_string(g_twtlInfo.engine.name.c_str()));
	json_object_set(root, "app", json_string(g_twtlInfo.engine.app.c_str()));
	json_object_set(root, "version", json_string(g_twtlInfo.engine.version.c_str()));

	while (req_node)
	{
		switch (req_node->type)
		{
		case PROTO_REQ_GET:
			JSON_ProtoReqGetProc(req_node, root, &res);
			break;
		case PROTO_REQ_SET:
			JSON_ProtoReqSetProc(req_node, root, &res);
			break;
		case PROTO_REQ_PUT:
			break;
		case PROTO_REQ_DELETE:
			break;
		case PROTO_REQ_DIFF:
			JSON_ProtoReqDiffProc(req_node, root, &res);
			break;
		case PROTO_REQ_PATCH:
			break;
		case PROTO_REQ_BETA:
			break;
		}

		req_node = req_node->next;
	}

	res = json_dumps(root, 0);
	json_decref(root);
	return res;
}
/*
void JSON_ProtoMakeResponse_bak(TWTL_PROTO_BUF* req, TWTL_PROTO_BUF* res)
{
	TWTL_PROTO_NODE* req_node = req->contents;
	memset(res, 0, sizeof(TWTL_PROTO_BUF));

	JSON_Init_ProtoBufHeader(res);

	while (req_node)
	{
		switch (req_node->type)
		{
		case PROTO_REQ_GET:
			JSON_ProtoReqGetProc(req_node, res);
			break;
		case PROTO_REQ_SET:
			JSON_ProtoReqSetProc(req_node, res);
			break;
		case PROTO_REQ_PUT:
			break;
		case PROTO_REQ_DELETE:
			break;
		case PROTO_REQ_DIFF:
			JSON_ProtoReqDiffProc(req_node, res);
			break;
		case PROTO_REQ_PATCH:
			break;
		case PROTO_REQ_BETA:
			break;
		}

		req_node = req_node->next;
	}
}
*/

void JSON_ProtoReqGetProc(TWTL_PROTO_NODE* req_node, json_t* root, char** res)
{
	json_t* contentsArray = json_array(); // "contents": [  ]  // []
	json_object_set(root, "contents", contentsArray);

	if (req_node->path.compare("/Engine/Name/") == 0)
	{
		json_t* contentNode = json_object(); // [ { } , { } , { }, ] // {}
		json_array_append(contentsArray, contentNode);
		
		json_object_set(contentNode, "type", json_string(PROTO_STR_RES_STATUS));
		json_object_set(contentNode, "path", json_string(req_node->path.c_str()));
		json_object_set(contentNode, "value", json_integer(PROTO_STATUS_SUCCESS));

		json_object_set(contentNode, "type", json_string(PROTO_STR_RES_OBJECT));
		json_object_set(contentNode, "path", json_string(req_node->path.c_str()));
		json_object_set(contentNode, "value", json_string(g_twtlInfo.engine.name.c_str()));
	}
	else if (req_node->path.compare("/Engine/Version/") == 0)
	{
		json_t* contentNode = json_object(); // [ { } , { } , { }, ] // {}
		json_array_append(contentsArray, contentNode);

		json_object_set(contentNode, "type", json_string(PROTO_STR_RES_STATUS));
		json_object_set(contentNode, "path", json_string(req_node->path.c_str()));
		json_object_set(contentNode, "value", json_integer(PROTO_STATUS_SUCCESS));

		json_object_set(contentNode, "type", json_string(PROTO_STR_RES_OBJECT));
		json_object_set(contentNode, "path", json_string(req_node->path.c_str()));
		json_object_set(contentNode, "value", json_string(g_twtlInfo.engine.version.c_str()));
	}
	else if (req_node->path.compare("/Engine/RequestPort/") == 0)
	{
		json_t* contentNode = json_object(); // [ { } , { } , { }, ] // {}
		json_array_append(contentsArray, contentNode);

		json_object_set(contentNode, "type", json_string(PROTO_STR_RES_STATUS));
		json_object_set(contentNode, "path", json_string(req_node->path.c_str()));
		json_object_set(contentNode, "value", json_integer(PROTO_STATUS_SUCCESS));

		json_object_set(contentNode, "type", json_string(PROTO_STR_RES_OBJECT));
		json_object_set(contentNode, "path", json_string(req_node->path.c_str()));
		json_object_set(contentNode, "value", json_integer(g_twtlInfo.engine.reqPort));
	}
	else if (req_node->path.compare("/Engine/TrapPort/") == 0)
	{ // This value must be set first
		if (g_twtlInfo.engine.trapPort == INVALID_PORT_VALUE)
		{ // Not set, return 400
			json_t* contentNode = json_object(); // [ { } , { } , { }, ] // {}
			json_array_append(contentsArray, contentNode);

			json_object_set(contentNode, "type", json_string(PROTO_STR_RES_STATUS));
			json_object_set(contentNode, "path", json_string(req_node->path.c_str()));
			json_object_set(contentNode, "value", json_integer(PROTO_STATUS_CLIENT_ERROR));
		}
		else
		{ // Return 200
			json_t* contentNode = json_object(); // [ { } , { } , { }, ] // {}
			json_array_append(contentsArray, contentNode);

			json_object_set(contentNode, "type", json_string(PROTO_STR_RES_STATUS));
			json_object_set(contentNode, "path", json_string(req_node->path.c_str()));
			json_object_set(contentNode, "value", json_integer(PROTO_STATUS_SUCCESS));

			json_object_set(contentNode, "type", json_string(PROTO_STR_RES_OBJECT));
			json_object_set(contentNode, "path", json_string(req_node->path.c_str()));
			json_object_set(contentNode, "value", json_integer(g_twtlInfo.engine.trapPort));
		}
	}
	else if (req_node->path.compare("/Reg/Short/") == 0)
	{
		BOOL failure = FALSE;

		TWTL_DB_REGISTRY* dbHkcuRun = (TWTL_DB_REGISTRY*)calloc(1, sizeof(TWTL_DB_REGISTRY));
		TWTL_DB_REGISTRY* dbHklmRun = (TWTL_DB_REGISTRY*)calloc(1, sizeof(TWTL_DB_REGISTRY));
		TWTL_DB_REGISTRY* dbHkcuRunOnce = (TWTL_DB_REGISTRY*)calloc(1, sizeof(TWTL_DB_REGISTRY));
		TWTL_DB_REGISTRY* dbHklmRunOnce = (TWTL_DB_REGISTRY*)calloc(1, sizeof(TWTL_DB_REGISTRY));
		TWTL_DB_SERVICE* dbServices = (TWTL_DB_SERVICE*)calloc(1, sizeof(TWTL_DB_SERVICE));
		
		if (!(dbHkcuRun && dbHklmRun && dbHkcuRunOnce && dbHklmRunOnce && dbServices))
		{ // DynAlloc Failure
			failure = TRUE;
		}

		if (!SnapCurrentStatus(NULL, dbHkcuRun, dbHklmRun, dbHkcuRunOnce, dbHklmRunOnce, dbServices, NULL, NULL, 0))
		{ // Snapshot Failure
			failure = TRUE;
		}

		if (failure)
		{ // Internal Server Error
			json_t* contentNode = json_object(); // [ { } , { } , { }, ] // {}
			json_array_append(contentsArray, contentNode);

			json_object_set(contentNode, "type", json_string(PROTO_STR_RES_STATUS));
			json_object_set(contentNode, "path", json_string(req_node->path.c_str()));
			json_object_set(contentNode, "value", json_integer(PROTO_STATUS_SERVER_ERROR));
		}
		else
		{ // Success
			json_t* contentNode = json_object(); // [ { } , { } , { }, ] // {}
			json_array_append(contentsArray, contentNode);

			json_object_set(contentNode, "type", json_string(PROTO_STR_RES_STATUS));
			json_object_set(contentNode, "path", json_string("/Reg/Short/"));
			json_object_set(contentNode, "value", json_integer(PROTO_STATUS_SUCCESS));

			json_object_set(contentNode, "type", json_string(PROTO_STR_RES_OBJECT));
			json_object_set(contentNode, "path", json_string("/Reg/Short/"));
			json_t* contentNodeObject = json_object();
			json_object_set(contentNode, "value", contentNodeObject);
			
			// Services
			{
				json_t* serviceRoot = json_object();
				json_object_set(contentNodeObject, "GlobalServices", serviceRoot);
				json_t* serviceArray = json_array();
				json_t* serviceNode = json_object();
				json_array_append(serviceArray, serviceNode);

				{
					char utf8_buf[DB_MAX_REG_PATH];
					WideCharToMultiByte(CP_UTF8, 0, dbServices->key, -1, utf8_buf, DB_MAX_REG_PATH, NULL, NULL);
					json_object_set(serviceNode, "name", json_string(utf8_buf));
				}
				if (dbServices->image_path[0] == L'\0')
				{
					char utf8_buf[DB_MAX_REG_PATH];
					json_object_set(serviceNode, "value", json_string(utf8_buf));
				}
				else
				{
					char utf8_buf[DB_MAX_REG_PATH];
					WideCharToMultiByte(CP_UTF8, 0, dbServices->image_path, -1, utf8_buf, DB_MAX_REG_PATH, NULL, NULL);
					json_object_set(serviceNode, "value", json_string(utf8_buf));
				}
			}

			// HKLM - Run
			{
				json_t* regRoot = json_object();
				json_object_set(contentNodeObject, "GlobalRun", regRoot);
				json_t* regArray = json_array();
				json_t* regNode = json_object();
				json_array_append(regArray, regNode);

				{
					char* utf8_buf = (char*) calloc(DB_MAX_REG_VALUE, sizeof(utf8_buf));
					WideCharToMultiByte(CP_UTF8, 0, dbHklmRun->value, -1, utf8_buf, DB_MAX_REG_VALUE, NULL, NULL);
					json_object_set(regNode, "name", json_string(utf8_buf));
					free(utf8_buf);
				}
				{
					char utf8_buf[DB_MAX_REG_NAME];
					WideCharToMultiByte(CP_UTF8, 0, dbHklmRun->name, -1, utf8_buf, DB_MAX_REG_NAME, NULL, NULL);
					json_object_set(regNode, "value", json_string(utf8_buf));
				}
			}

			// HKLM - RunOnce
			{
				json_t* regRoot = json_object();
				json_object_set(contentNodeObject, "GlobalRunOnce", regRoot);
				json_t* regArray = json_array();
				json_t* regNode = json_object();
				json_array_append(regArray, regNode);

				{
					char* utf8_buf = (char*)calloc(DB_MAX_REG_VALUE, sizeof(utf8_buf));
					WideCharToMultiByte(CP_UTF8, 0, dbHklmRunOnce->value, -1, utf8_buf, DB_MAX_REG_VALUE, NULL, NULL);
					json_object_set(regNode, "name", json_string(utf8_buf));
					free(utf8_buf);
				}
				{
					char utf8_buf[DB_MAX_REG_NAME];
					WideCharToMultiByte(CP_UTF8, 0, dbHklmRunOnce->name, -1, utf8_buf, DB_MAX_REG_NAME, NULL, NULL);
					json_object_set(regNode, "value", json_string(utf8_buf));
				}
			}

			// HKCU - Run
			{
				json_t* regRoot = json_object();
				json_object_set(contentNodeObject, "UserRun", regRoot);
				json_t* regArray = json_array();
				json_t* regNode = json_object();
				json_array_append(regArray, regNode);

				{
					char* utf8_buf = (char*)calloc(DB_MAX_REG_VALUE, sizeof(utf8_buf));
					WideCharToMultiByte(CP_UTF8, 0, dbHkcuRun->value, -1, utf8_buf, DB_MAX_REG_VALUE, NULL, NULL);
					json_object_set(regNode, "name", json_string(utf8_buf));
					free(utf8_buf);
				}
				{
					char utf8_buf[DB_MAX_REG_NAME];
					WideCharToMultiByte(CP_UTF8, 0, dbHkcuRun->name, -1, utf8_buf, DB_MAX_REG_NAME, NULL, NULL);
					json_object_set(regNode, "value", json_string(utf8_buf));
				}
			}

			// HKCU - RunOnce
			{
				json_t* regRoot = json_object();
				json_object_set(contentNodeObject, "UserRunOnce", regRoot);
				json_t* regArray = json_array();
				json_t* regNode = json_object();
				json_array_append(regArray, regNode);

				{
					char* utf8_buf = (char*)calloc(DB_MAX_REG_VALUE, sizeof(utf8_buf));
					WideCharToMultiByte(CP_UTF8, 0, dbHkcuRunOnce->value, -1, utf8_buf, DB_MAX_REG_VALUE, NULL, NULL);
					json_object_set(regNode, "name", json_string(utf8_buf));
					free(utf8_buf);
				}
				{
					char utf8_buf[DB_MAX_REG_NAME];
					WideCharToMultiByte(CP_UTF8, 0, dbHkcuRunOnce->name, -1, utf8_buf, DB_MAX_REG_NAME, NULL, NULL);
					json_object_set(regNode, "value", json_string(utf8_buf));
				}
			}
		}

		// Finalize
		free(dbHkcuRun);
		free(dbHklmRun);
		free(dbHkcuRunOnce);
		free(dbHklmRunOnce);
		free(dbServices);
		dbHkcuRun = NULL;
		dbHklmRun = NULL;
		dbHkcuRunOnce = NULL;
		dbHklmRunOnce = NULL;
		dbServices = NULL;
	}
}

void JSON_ProtoReqSetProc(TWTL_PROTO_NODE* req_node, json_t* root, char** res)
{
	json_t* contentsArray = json_array(); // "contents": [  ]  // []
	json_object_set(root, "contents", contentsArray);

	if (req_node->path.compare("/Engine/Name/") == 0)
	{ // Const value, produce error
		json_t* contentNode = json_object(); // [ { } , { } , { }, ] // {}
		json_array_append(contentsArray, contentNode);

		json_object_set(contentNode, "type", json_string(PROTO_STR_RES_STATUS));
		json_object_set(contentNode, "path", json_string(req_node->path.c_str()));
		json_object_set(contentNode, "value", json_integer(PROTO_STATUS_CLIENT_ERROR));
	}
	else if (req_node->path.compare("/Engine/Version/") == 0)
	{ // Const value, produce error
		json_t* contentNode = json_object(); // [ { } , { } , { }, ] // {}
		json_array_append(contentsArray, contentNode);

		json_object_set(contentNode, "type", json_string(PROTO_STR_RES_STATUS));
		json_object_set(contentNode, "path", json_string(req_node->path.c_str()));
		json_object_set(contentNode, "value", json_integer(PROTO_STATUS_CLIENT_ERROR));
	}
	else if (req_node->path.compare("/Engine/RequsetPort/") == 0)
	{ // Const value, produce error
		json_t* contentNode = json_object(); // [ { } , { } , { }, ] // {}
		json_array_append(contentsArray, contentNode);

		json_object_set(contentNode, "type", json_string(PROTO_STR_RES_STATUS));
		json_object_set(contentNode, "path", json_string(req_node->path.c_str()));
		json_object_set(contentNode, "value", json_integer(PROTO_STATUS_CLIENT_ERROR));
	}
	else if (req_node->path.compare("/Engine/TrapPort/") == 0)
	{ // 
		json_t* contentNode = json_object(); // [ { } , { } , { }, ] // {}
		json_array_append(contentsArray, contentNode);

		g_twtlInfo.engine.trapPort = (SHORT)req_node->value_int32;

		json_object_set(contentNode, "type", json_string(PROTO_STR_RES_STATUS));
		json_object_set(contentNode, "path", json_string(req_node->path.c_str()));
		json_object_set(contentNode, "value", json_integer(PROTO_STATUS_SUCCESS));

		if (g_twtlInfo.engine.trapPort == 0) {} // Connection Teardown

		trapPort = g_twtlInfo.engine.trapPort;
	}
}

void JSON_ProtoReqDiffProc(TWTL_PROTO_NODE* req_node, json_t* root, char** res)
{
	if (req_node->path.compare("/Reg/Short/") == 0)
	{

	}
}

json_t* JSON_ProtoBufToJson(TWTL_PROTO_BUF* res)
{
	json_t* root = json_object();
	json_t* json_arr = json_array();

	TWTL_PROTO_NODE* res_node = res->contents;

	json_object_set(root, "name", json_string(res->name.c_str()));
	json_object_set(root, "app", json_string(res->app.c_str()));
	json_object_set(root, "version", json_string(res->version.c_str()));

	while (res_node)
	{
		json_t* json_node = json_object();

		switch (res_node->type)
		{
		case PROTO_REQ_GET:
			json_object_set(json_node, "type", json_string("request.get"));
			break;
		case PROTO_REQ_SET:
			json_object_set(json_node, "type", json_string("request.set"));
			break;
		case PROTO_RES_STATUS:
			json_object_set(json_node, "type", json_string("response.status"));
			break;
		case PROTO_RES_OBJECT:
			json_object_set(json_node, "type", json_string("response.object"));
			break;
		}

		json_object_set(json_node, "path", json_string(res_node->path.c_str()));
		
		switch (res_node->value_type)
		{
		case PROTO_VALUE_STRING:
			json_object_set(json_node, "value", json_string(res_node->value_string.c_str()));
			break;
		case PROTO_VALUE_INT32:
			json_object_set(json_node, "value", json_integer(res_node->value_int32));
			break;
		case PROTO_VALUE_FLOAT32:
			json_object_set(json_node, "value", json_real(res_node->value_real));
			break;
		case PROTO_VALUE_BOOLEAN:
			json_object_set(json_node, "value", json_boolean(res_node->value_boolean));
			break;
		case PROTO_VALUE_NULL:
			break;
		}

		json_array_append(json_arr, json_node);

		res_node = res_node->next;
	}
	json_object_set(root, "contents", json_arr);

	return root;
}


void JSON_Init_TWTL_INFO_DATA()
{
	JSON_Init_TWTL_INFO_ENGINE_NODE(&(g_twtlInfo.engine));
}


void JSON_Init_TWTL_INFO_ENGINE_NODE(TWTL_INFO_ENGINE_NODE* node)
{
	node->name = "TWTL";
	node->app = "TWTL-Engine";
	node->version = "1.0";
	node->reqPort = 5259;
	node->trapPort = INVALID_PORT_VALUE; // Invalid value
}


void JSON_Init_ProtoBufHeader(TWTL_PROTO_BUF* buf)
{
	buf->name = g_twtlInfo.engine.name;
	buf->app = g_twtlInfo.engine.app;
	buf->version = g_twtlInfo.engine.version;
}