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
	// root = json_loadb(buf, buflen, 0, &error);

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
				StringCchCopyA(req->name, TWTL_PROTO_MAX_BUF, json_string_value(element));
			else if (StrCmpIA(key, "app") == 0)
				StringCchCopyA(req->app, TWTL_PROTO_MAX_BUF, json_string_value(element));
			else if (StrCmpIA(key, "version") == 0)
				StringCchCopyA(req->version, TWTL_PROTO_MAX_BUF, json_string_value(element));
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
				if (StrCmpIA(json_string_value(element), "request.get") == 0
					|| StrCmpIA(json_string_value(element), "get.request") == 0)
					node->type = PROTO_REQ_GET;
				else if (StrCmpIA(json_string_value(element), "request.set") == 0
					|| StrCmpIA(json_string_value(element), "set.request") == 0)
					node->type = PROTO_REQ_SET;
				else if (StrCmpIA(json_string_value(element), "response.status") == 0)
					node->type = PROTO_RES_STATUS;
				else if (StrCmpIA(json_string_value(element), "response.object") == 0)
					node->type = PROTO_RES_OBJECT;
			}
			else if (StrCmpIA(key, "path") == 0)
				StringCchCopyA(node->path, TWTL_PROTO_MAX_BUF, json_string_value(element));
			else if (StrCmpIA(key, "value") == 0)
			{
				node->value_type = PROTO_VALUE_STR;
				StringCchCopyA(node->value_str, TWTL_PROTO_MAX_BUF, json_string_value(element));
			}
			break;
		case JSON_INTEGER:
			if (StrCmpIA(key, "value") == 0)
			{
				node->value_type = PROTO_VALUE_INT;
				node->value_int = json_integer_value(element);
			}
			break;
		case JSON_REAL:
			if (StrCmpIA(key, "value") == 0)
			{
				node->value_type = PROTO_VALUE_REAL;
				node->value_real = json_real_value(element);
			}
			break;
		case JSON_TRUE:
			if (StrCmpIA(key, "value") == 0)
			{
				node->value_type = PROTO_VALUE_BOOL;
				node->value_bool = TRUE;
			}
			break;
		case JSON_FALSE:
			if (StrCmpIA(key, "value") == 0)
			{
				node->value_type = PROTO_VALUE_BOOL;
				node->value_bool = FALSE;
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

void JSON_ProtoMakeResponse(TWTL_PROTO_BUF* req, TWTL_PROTO_BUF* res)
{
	TWTL_PROTO_NODE* req_node = req->contents;
	memset(res, 0, sizeof(TWTL_PROTO_BUF));

	StringCchCopyA(res->name, TWTL_PROTO_MAX_BUF, "TWTL");
	StringCchCopyA(res->app, TWTL_PROTO_MAX_BUF, "TWTL-Engine");
	StringCchCopyA(res->version, TWTL_PROTO_MAX_BUF, "1");

	while (req_node)
	{
		switch (req_node->type)
		{
		case PROTO_REQ_GET:
			if (StrCmpIA(req_node->path, "/Engine/Name/") == 0)
			{
				TWTL_PROTO_NODE* res_node_status = JSON_AddProtoNode(res);
				res_node_status->type = PROTO_RES_STATUS;
				StringCchCopyA(res_node_status->path, TWTL_PROTO_MAX_BUF, req_node->path);
				res_node_status->value_type = PROTO_VALUE_INT;
				res_node_status->value_int = PROTO_STATUS_SUCCESS;

				TWTL_PROTO_NODE* res_node_object = JSON_AddProtoNode(res);
				res_node_object->type = PROTO_RES_OBJECT;
				StringCchCopyA(res_node_object->path, TWTL_PROTO_MAX_BUF, req_node->path);
				res_node_object->value_type = PROTO_VALUE_STR;
				StringCchCopyA(res_node_object->value_str, TWTL_PROTO_MAX_BUF, g_twtlInfo.engine.name);
			}
			else if (StrCmpIA(req_node->path, "/Engine/Version/") == 0)
			{
				TWTL_PROTO_NODE* res_node_status = JSON_AddProtoNode(res);
				res_node_status->type = PROTO_RES_STATUS;
				StringCchCopyA(res_node_status->path, TWTL_PROTO_MAX_BUF, req_node->path);
				res_node_status->value_type = PROTO_VALUE_INT;
				res_node_status->value_int = PROTO_STATUS_SUCCESS;

				TWTL_PROTO_NODE* res_node_object = JSON_AddProtoNode(res);
				res_node_object->type = PROTO_RES_OBJECT;
				StringCchCopyA(res_node_object->path, TWTL_PROTO_MAX_BUF, req_node->path);
				res_node_object->value_type = PROTO_VALUE_STR;
				StringCchCopyA(res_node_object->value_str, TWTL_PROTO_MAX_BUF, g_twtlInfo.engine.version);
			}
			else if (StrCmpIA(req_node->path, "/Engine/RequestPort/") == 0)
			{
				TWTL_PROTO_NODE* res_node_status = JSON_AddProtoNode(res);
				res_node_status->type = PROTO_RES_STATUS;
				StringCchCopyA(res_node_status->path, TWTL_PROTO_MAX_BUF, req_node->path);
				res_node_status->value_type = PROTO_VALUE_INT;
				res_node_status->value_int = PROTO_STATUS_SUCCESS;

				TWTL_PROTO_NODE* res_node_object = JSON_AddProtoNode(res);
				res_node_object->type = PROTO_RES_OBJECT;
				StringCchCopyA(res_node_object->path, TWTL_PROTO_MAX_BUF, req_node->path);
				res_node_object->value_type = PROTO_VALUE_INT;
				res_node_object->value_int = g_twtlInfo.engine.reqPort;
			}
			else if (StrCmpIA(req_node->path, "/Engine/TrapPort/") == 0)
			{ // This value must be set first
				if (g_twtlInfo.engine.trapPort == INVALID_PORT_VALUE)
				{ // Not set, return 400
					TWTL_PROTO_NODE* res_node_status = JSON_AddProtoNode(res);
					res_node_status->type = PROTO_RES_STATUS;
					StringCchCopyA(res_node_status->path, TWTL_PROTO_MAX_BUF, req_node->path);
					res_node_status->value_type = PROTO_VALUE_INT;
					res_node_status->value_int = PROTO_STATUS_CLIENT_ERROR;
				}
				else
				{ // Return 200
					TWTL_PROTO_NODE* res_node_status = JSON_AddProtoNode(res);
					res_node_status->type = PROTO_RES_STATUS;
					StringCchCopyA(res_node_status->path, TWTL_PROTO_MAX_BUF, req_node->path);
					res_node_status->value_type = PROTO_VALUE_INT;
					res_node_status->value_int = PROTO_STATUS_SUCCESS;

					TWTL_PROTO_NODE* res_node_object = JSON_AddProtoNode(res);
					res_node_object->type = PROTO_RES_OBJECT;
					StringCchCopyA(res_node_object->path, TWTL_PROTO_MAX_BUF, req_node->path);
					res_node_object->value_type = PROTO_VALUE_INT;
					res_node_object->value_int = g_twtlInfo.engine.trapPort;
				}				
			}
			break;
		case PROTO_REQ_SET:
			if (StrCmpIA(req_node->path, "/Engine/Name/") == 0)
			{ // Const value, produce error
				TWTL_PROTO_NODE* res_node_status = JSON_AddProtoNode(res);
				res_node_status->type = PROTO_RES_STATUS;
				StringCchCopyA(res_node_status->path, TWTL_PROTO_MAX_BUF, req_node->path);
				res_node_status->value_type = PROTO_VALUE_INT;
				res_node_status->value_int = PROTO_STATUS_CLIENT_ERROR;
			}
			else if (StrCmpIA(req_node->path, "/Engine/Version/") == 0)
			{ // Const value, produce error
				TWTL_PROTO_NODE* res_node_status = JSON_AddProtoNode(res);
				res_node_status->type = PROTO_RES_STATUS;
				StringCchCopyA(res_node_status->path, TWTL_PROTO_MAX_BUF, req_node->path);
				res_node_status->value_type = PROTO_VALUE_INT;
				res_node_status->value_int = PROTO_STATUS_CLIENT_ERROR;
			}
			else if (StrCmpIA(req_node->path, "/Engine/RequsetPort/") == 0)
			{ // Const value, produce error
				TWTL_PROTO_NODE* res_node_status = JSON_AddProtoNode(res);
				res_node_status->type = PROTO_RES_STATUS;
				StringCchCopyA(res_node_status->path, TWTL_PROTO_MAX_BUF, req_node->path);
				res_node_status->value_type = PROTO_VALUE_INT;
				res_node_status->value_int = PROTO_STATUS_CLIENT_ERROR;
			}
			else if (StrCmpIA(req_node->path, "/Engine/TrapPort/") == 0)
			{ // 
				g_twtlInfo.engine.trapPort = (SHORT) req_node->value_int;

				TWTL_PROTO_NODE* res_node_status = JSON_AddProtoNode(res);
				res_node_status->type = PROTO_RES_STATUS;
				StringCchCopyA(res_node_status->path, TWTL_PROTO_MAX_BUF, req_node->path);
				res_node_status->value_type = PROTO_VALUE_INT;
				res_node_status->value_int = PROTO_STATUS_SUCCESS;

				trapPort = g_twtlInfo.engine.trapPort;
			}
			break;
		}

		req_node = req_node->next;
	}
}

json_t* JSON_ProtoBufToJson(TWTL_PROTO_BUF* res)
{
	json_t* root = json_object();
	json_t* json_arr = json_array();

	TWTL_PROTO_NODE* res_node = res->contents;

	json_object_set_new(root, "name", json_string(res->name));
	json_object_set_new(root, "app", json_string(res->app));
	json_object_set_new(root, "version", json_string(res->version));

	while (res_node)
	{
		json_t* json_node = json_object();

		switch (res_node->type)
		{
		case PROTO_REQ_GET:
			json_object_set_new(json_node, "type", json_string("request.get"));
			break;
		case PROTO_REQ_SET:
			json_object_set_new(json_node, "type", json_string("request.set"));
			break;
		case PROTO_RES_STATUS:
			json_object_set_new(json_node, "type", json_string("response.status"));
			break;
		case PROTO_RES_OBJECT:
			json_object_set_new(json_node, "type", json_string("response.object"));
			break;
		}

		json_object_set_new(json_node, "path", json_string(res_node->path));
		
		switch (res_node->value_type)
		{
		case PROTO_VALUE_STR:
			json_object_set_new(json_node, "value", json_string(res_node->value_str));
			break;
		case PROTO_VALUE_INT:
			json_object_set_new(json_node, "value", json_integer(res_node->value_int));
			break;
		case PROTO_VALUE_REAL:
			json_object_set_new(json_node, "value", json_real(res_node->value_real));
			break;
		case PROTO_VALUE_BOOL:
			json_object_set_new(json_node, "value", json_boolean(res_node->value_bool));
			break;
		case PROTO_VALUE_NULL:
			break;
		}

		json_array_append(json_arr, json_node);

		res_node = res_node->next;
	}
	json_object_set_new(root, "contents", json_arr);

	return root;
}


void JSON_Init_TWTL_INFO_DATA()
{
	JSON_Init_TWTL_INFO_ENGINE_NODE(&(g_twtlInfo.engine));
}


void JSON_Init_TWTL_INFO_ENGINE_NODE(TWTL_INFO_ENGINE_NODE* node)
{
	StringCchCopyA(node->name, TWTL_PROTO_MAX_BUF, "TWTL");
	StringCchCopyA(node->version, TWTL_PROTO_MAX_BUF, "1.0");
	node->reqPort = 5259;
	node->trapPort = INVALID_PORT_VALUE; // Invalid value
}

