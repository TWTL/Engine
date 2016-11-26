// TWTL_JSON.cpp : DLL 응용 프로그램을 위해 내보낸 함수를 정의합니다.
//

#include "stdafx.h"

#include "JsonFunc.h"

/*
* Parse text into a JSON object. If text is valid JSON, returns a
* json_t structure, otherwise prints and error and returns null.
*/
#define MAX_CHARS 4096


static HANDLE g_hMutex;
extern TWTL_INFO_DATA g_twtlInfo;
extern TWTL_TRAP_QUEUE trapQueue;
extern SHORT trapPort;
extern sqlite3 *g_db;

// Must call JSON_ClearNode outside
DWORD JSON_Parse(const char buf[], size_t buflen, TWTL_PROTO_BUF* req)
{
	/* parse text into JSON structure */
	json_t *root;
	json_error_t error;
	memset(req, 0, sizeof(TWTL_PROTO_BUF));

	// For debug
#ifdef _DEBUG
	FILE* fp = NULL;
	fopen_s(&fp, "json_log.txt", "a");
	fprintf(fp, "[E <- U] MainPort Receive\n%s\n\n", buf);
	fclose(fp);
#endif

	printf("[E <- U] MainPort Receive\n");
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

BOOL JSON_EnqTrapQueue(TWTL_TRAP_QUEUE* queue, std::string inPath)
{
	DWORD dwWaitResult = WaitForSingleObject(g_hMutex, INFINITE);
	
	TWTL_TRAP_QUEUE_NODE** node = NULL;
	switch (dwWaitResult)
	{
		// The thread got ownership of the mutex
	case WAIT_OBJECT_0:
		// TODO: Write to the database
		node = &(queue->node);
		while (*node != NULL)
			node = &((*node)->next);

		(*node) = (TWTL_TRAP_QUEUE_NODE*)malloc(sizeof(TWTL_TRAP_QUEUE_NODE));
		(*node)->path = inPath;
		(*node)->next = NULL;
		queue->count++;
		
		// Release ownership of the mutex object
		if (!ReleaseMutex(g_hMutex))
		{
			// Handle error.
		}
		break;
		// The thread got ownership of an abandoned mutex
		// The database is in an indeterminate state
	case WAIT_ABANDONED:
		return TRUE;
		break;
	}

	return FALSE;
}

BOOL JSON_DeqTrapQueue(TWTL_TRAP_QUEUE* queue, std::string* outBuf)
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
				*outBuf = queue->node->path;
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

	json_object_set(root, "app", json_string(g_twtlInfo.engine.app.c_str()));
	json_object_set(root, "name", json_string(g_twtlInfo.engine.name.c_str()));
	json_object_set(root, "version", json_string(g_twtlInfo.engine.version.c_str()));

	while (req_node)
	{
		switch (req_node->type)
		{
		case PROTO_REQ_GET:
			JSON_ProtoReqGetProc(req_node, root);
			break;
		case PROTO_REQ_SET:
			JSON_ProtoReqSetProc(req_node, root);
			break;
		case PROTO_REQ_PUT:
			break;
		case PROTO_REQ_DELETE:
			break;
		case PROTO_REQ_DIFF:
			JSON_ProtoReqDiffProc(req_node, root);
			break;
		case PROTO_REQ_PATCH:
			// {"app":"TWTL-GUI","name":"TWTL","version":"1","contents":[{"type":"request.patch","path":"/Reg/Short/GlobalServices/","value":{"value":[{"Name":".NET CLR Data","Value":null}],"accept":[false],"reject":[true]}}]}
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

void JSON_ProtoReqGetProc(TWTL_PROTO_NODE* req_node, json_t* root)
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

		// Query struct size
		DWORD structSize[8] = { 0 };

		if (!(dbHkcuRun && dbHklmRun && dbHkcuRunOnce && dbHklmRunOnce && dbServices))
		{ // DynAlloc Failure
			failure = TRUE;
		}

		if (!SnapCurrentStatus(NULL, dbHkcuRun, dbHklmRun, dbHkcuRunOnce, dbHklmRunOnce, dbServices, NULL, NULL, structSize, 2))
		{ // Snapshot Failure
			failure = TRUE;
		}

		dbHkcuRun = (TWTL_DB_REGISTRY*)realloc(dbHkcuRun, sizeof(TWTL_DB_REGISTRY)*(structSize[1] + 1));
		memset(dbHkcuRun, 0x00, sizeof(TWTL_DB_REGISTRY)*(structSize[1] + 1));
		dbHklmRun = (TWTL_DB_REGISTRY*)realloc(dbHklmRun, sizeof(TWTL_DB_REGISTRY)*(structSize[2] + 1));
		memset(dbHklmRun, 0x00, sizeof(TWTL_DB_REGISTRY)*(structSize[2] + 1));
		dbHkcuRunOnce = (TWTL_DB_REGISTRY*)realloc(dbHkcuRunOnce, sizeof(TWTL_DB_REGISTRY)*(structSize[3] + 1));
		memset(dbHkcuRunOnce, 0x00, sizeof(TWTL_DB_REGISTRY)*(structSize[3] + 1));
		dbHklmRunOnce = (TWTL_DB_REGISTRY*)realloc(dbHklmRunOnce, sizeof(TWTL_DB_REGISTRY)*(structSize[4] + 1));
		memset(dbHklmRunOnce, 0x00, sizeof(TWTL_DB_REGISTRY)*(structSize[4] + 1));
		dbServices = (TWTL_DB_SERVICE*)realloc(dbServices, sizeof(TWTL_DB_SERVICE)*(structSize[5] + 1));
		memset(dbServices, 0x00, sizeof(TWTL_DB_SERVICE)*(structSize[5] + 1));
		
		if (!SnapCurrentStatus(NULL, dbHkcuRun, dbHklmRun, dbHkcuRunOnce, dbHklmRunOnce, dbServices, NULL, NULL, NULL, 0))
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
			{ // ArraySize == structSize[5]
				json_t* serviceRoot = json_array();
				json_object_set(contentNodeObject, "GlobalServices", serviceRoot);
				
				for (DWORD i = 0; i < structSize[5]; i++)
				{
					json_t* serviceNode = json_object();
					json_array_append(serviceRoot, serviceNode);

					{
						char utf8_buf[DB_MAX_REG_PATH];
						WideCharToMultiByte(CP_UTF8, 0, dbServices->key, -1, utf8_buf, DB_MAX_REG_PATH, NULL, NULL);
						json_object_set(serviceNode, "Name", json_string(utf8_buf));
					}
					if (dbServices->image_path[0] == L'\0')
					{
						char utf8_buf[DB_MAX_REG_PATH];
						json_object_set(serviceNode, "Value", json_string(utf8_buf));
					}
					else
					{
						char utf8_buf[DB_MAX_REG_PATH];
						WideCharToMultiByte(CP_UTF8, 0, dbServices->image_path, -1, utf8_buf, DB_MAX_REG_PATH, NULL, NULL);
						json_object_set(serviceNode, "Value", json_string(utf8_buf));
					}
				}				
			}

			// HKLM - Run
			{
				json_t* regRoot = json_object();
				json_object_set(contentNodeObject, "GlobalRun", regRoot);

				for (DWORD i = 0; i < structSize[2]; i++)
				{
					json_t* regNode = json_object();
					json_array_append(regRoot, regNode);

					{
						char* utf8_buf = (char*)calloc(DB_MAX_REG_VALUE, sizeof(utf8_buf));
						WideCharToMultiByte(CP_UTF8, 0, dbHklmRun->value, -1, utf8_buf, DB_MAX_REG_VALUE, NULL, NULL);
						json_object_set(regNode, "Name", json_string(utf8_buf));
						free(utf8_buf);
					}
					{
						char utf8_buf[DB_MAX_REG_NAME];
						WideCharToMultiByte(CP_UTF8, 0, dbHklmRun->name, -1, utf8_buf, DB_MAX_REG_NAME, NULL, NULL);
						json_object_set(regNode, "Value", json_string(utf8_buf));
					}
				}
			}

			// HKLM - RunOnce
			{
				json_t* regRoot = json_object();
				json_object_set(contentNodeObject, "GlobalRunOnce", regRoot);

				for (DWORD i = 0; i < structSize[4]; i++)
				{
					json_t* regNode = json_object();
					json_array_append(regRoot, regNode);

					{
						char* utf8_buf = (char*)calloc(DB_MAX_REG_VALUE, sizeof(utf8_buf));
						WideCharToMultiByte(CP_UTF8, 0, dbHklmRunOnce->value, -1, utf8_buf, DB_MAX_REG_VALUE, NULL, NULL);
						json_object_set(regNode, "Name", json_string(utf8_buf));
						free(utf8_buf);
					}
					{
						char utf8_buf[DB_MAX_REG_NAME];
						WideCharToMultiByte(CP_UTF8, 0, dbHklmRunOnce->name, -1, utf8_buf, DB_MAX_REG_NAME, NULL, NULL);
						json_object_set(regNode, "Value", json_string(utf8_buf));
					}
				}
			}

			// HKCU - Run
			{
				json_t* regRoot = json_object();
				json_object_set(contentNodeObject, "UserRun", regRoot);

				for (DWORD i = 0; i < structSize[1]; i++)
				{
					json_t* regNode = json_object();
					json_array_append(regRoot, regNode);

					{
						char* utf8_buf = (char*)calloc(DB_MAX_REG_VALUE, sizeof(utf8_buf));
						WideCharToMultiByte(CP_UTF8, 0, dbHkcuRun->value, -1, utf8_buf, DB_MAX_REG_VALUE, NULL, NULL);
						json_object_set(regNode, "Name", json_string(utf8_buf));
						free(utf8_buf);
					}
					{
						char utf8_buf[DB_MAX_REG_NAME];
						WideCharToMultiByte(CP_UTF8, 0, dbHkcuRun->name, -1, utf8_buf, DB_MAX_REG_NAME, NULL, NULL);
						json_object_set(regNode, "Value", json_string(utf8_buf));
					}
				}
			}

			// HKCU - RunOnce
			{
				json_t* regRoot = json_object();
				json_object_set(contentNodeObject, "UserRun", regRoot);

				for (DWORD i = 0; i < structSize[3]; i++)
				{
					json_t* regNode = json_object();
					json_array_append(regRoot, regNode);

					{
						char* utf8_buf = (char*)calloc(DB_MAX_REG_VALUE, sizeof(utf8_buf));
						WideCharToMultiByte(CP_UTF8, 0, dbHkcuRunOnce->value, -1, utf8_buf, DB_MAX_REG_VALUE, NULL, NULL);
						json_object_set(regNode, "Name", json_string(utf8_buf));
						free(utf8_buf);
					}
					{
						char utf8_buf[DB_MAX_REG_NAME];
						WideCharToMultiByte(CP_UTF8, 0, dbHkcuRunOnce->name, -1, utf8_buf, DB_MAX_REG_NAME, NULL, NULL);
						json_object_set(regNode, "Value", json_string(utf8_buf));
					}
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

void JSON_ProtoReqSetProc(TWTL_PROTO_NODE* req_node, json_t* root)
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

void JSON_ProtoReqDiffProc(TWTL_PROTO_NODE* req_node, json_t* root)
{
	if (req_node->path.compare("/Reg/Short/GlobalServices/") == 0)
	{
		JSON_DiffProc_RegShort(req_node, root, REG_SHORT_SERVICE);
	}
	else if (req_node->path.compare("/Reg/Short/GlobalRun/") == 0)
	{
		JSON_DiffProc_RegShort(req_node, root, REG_SHORT_HKLM_RUN);
	}
	else if (req_node->path.compare("/Reg/Short/GlobalRunOnce/") == 0)
	{
		JSON_DiffProc_RegShort(req_node, root, REG_SHORT_HKLM_RUNONCE);
	}
	else if (req_node->path.compare("/Reg/Short/UserRun/") == 0)
	{
		JSON_DiffProc_RegShort(req_node, root, REG_SHORT_HKCU_RUN);
	}
	else if (req_node->path.compare("/Reg/Short/UserRunOnce/") == 0)
	{
		JSON_DiffProc_RegShort(req_node, root, REG_SHORT_HKCU_RUNONCE);
	}
}

// BOOL onHkcuRun, onHklmRun, onHkcuRunOnce, onHklmRunOnce, onSerives
// Only one of these value can be true
void JSON_DiffProc_RegShort(TWTL_PROTO_NODE* req_node, json_t* root, TWTL_REG_SHORT_TYPE type)
{
	json_t* contentsArray = json_array(); // "contents": [  ]  // []
	json_object_set(root, "contents", contentsArray);

	BOOL failure = FALSE;

	// Get database's row count
	int countLastHkcuRun = 0;
	int countLastHklmRun = 0;
	int countLastHkcuRunOnce = 0;
	int countLastHklmRunOnce = 0;
	int countLastServices = 0;
	DB_SelectQuery(g_db, DB_REG_HKCU_RUN, NULL, &countLastHkcuRun, NULL);
	DB_SelectQuery(g_db, DB_REG_HKLM_RUN, NULL, &countLastHklmRun, NULL);
	DB_SelectQuery(g_db, DB_REG_HKCU_RUNONCE, NULL, &countLastHkcuRunOnce, NULL);
	DB_SelectQuery(g_db, DB_REG_HKCU_RUNONCE, NULL, &countLastHklmRunOnce, NULL);
	DB_SelectQuery(g_db, DB_SERVICE, NULL, &countLastServices, NULL);

	TWTL_DB_REGISTRY* lastHkcuRun = (TWTL_DB_REGISTRY*)calloc(countLastHkcuRun + 1, sizeof(TWTL_DB_REGISTRY));
	TWTL_DB_REGISTRY* lastHklmRun = (TWTL_DB_REGISTRY*)calloc(countLastHklmRun + 1, sizeof(TWTL_DB_REGISTRY));
	TWTL_DB_REGISTRY* lastHkcuRunOnce = (TWTL_DB_REGISTRY*)calloc(countLastHkcuRunOnce + 1, sizeof(TWTL_DB_REGISTRY));
	TWTL_DB_REGISTRY* lastHklmRunOnce = (TWTL_DB_REGISTRY*)calloc(countLastHklmRunOnce + 1, sizeof(TWTL_DB_REGISTRY));
	TWTL_DB_SERVICE* lastServices = (TWTL_DB_SERVICE*)calloc(countLastServices + 1, sizeof(TWTL_DB_SERVICE));

	if (!(lastHkcuRun && lastHklmRun && lastHkcuRunOnce && lastHklmRunOnce && lastServices))
	{ // DynAlloc Failure
		failure = TRUE;
	}

	// Query Last Data
	DB_SelectQuery(g_db, DB_REG_HKCU_RUN, lastHkcuRun, &countLastHkcuRun, NULL);
	DB_SelectQuery(g_db, DB_REG_HKLM_RUN, lastHklmRun, &countLastHklmRun, NULL);
	DB_SelectQuery(g_db, DB_REG_HKCU_RUNONCE, lastHkcuRunOnce, &countLastHkcuRunOnce, NULL);
	DB_SelectQuery(g_db, DB_REG_HKCU_RUNONCE, lastHklmRunOnce, &countLastHklmRunOnce, NULL);
	DB_SelectQuery(g_db, DB_SERVICE, lastServices, &countLastServices, NULL);


	// Query Current Data
	TWTL_DB_REGISTRY* nowHkcuRun = (TWTL_DB_REGISTRY*)calloc(1, sizeof(TWTL_DB_REGISTRY));
	TWTL_DB_REGISTRY* nowHklmRun = (TWTL_DB_REGISTRY*)calloc(1, sizeof(TWTL_DB_REGISTRY));
	TWTL_DB_REGISTRY* nowHkcuRunOnce = (TWTL_DB_REGISTRY*)calloc(1, sizeof(TWTL_DB_REGISTRY));
	TWTL_DB_REGISTRY* nowHklmRunOnce = (TWTL_DB_REGISTRY*)calloc(1, sizeof(TWTL_DB_REGISTRY));
	TWTL_DB_SERVICE* nowServices = (TWTL_DB_SERVICE*)calloc(1, sizeof(TWTL_DB_SERVICE));
	// Query struct size
	DWORD structSize[8] = { 0 };
	if (!(nowHkcuRun && nowHklmRun && nowHkcuRunOnce && nowHklmRunOnce && nowServices))
	{ // DynAlloc Failure
		failure = TRUE;
	}
	if (!SnapCurrentStatus(NULL, nowHkcuRun, nowHklmRun, nowHkcuRunOnce, nowHklmRunOnce, nowServices, NULL, NULL, structSize, 2))
	{ // Snapshot Failure
		failure = TRUE;
	}
	nowHkcuRun = (TWTL_DB_REGISTRY*)realloc(nowHkcuRun, sizeof(TWTL_DB_REGISTRY)*(structSize[1] + 1));
	memset(nowHkcuRun, 0x00, sizeof(TWTL_DB_REGISTRY)*(structSize[1] + 1));
	nowHklmRun = (TWTL_DB_REGISTRY*)realloc(nowHklmRun, sizeof(TWTL_DB_REGISTRY)*(structSize[2] + 1));
	memset(nowHklmRun, 0x00, sizeof(TWTL_DB_REGISTRY)*(structSize[2] + 1));
	nowHkcuRunOnce = (TWTL_DB_REGISTRY*)realloc(nowHkcuRunOnce, sizeof(TWTL_DB_REGISTRY)*(structSize[3] + 1));
	memset(nowHkcuRunOnce, 0x00, sizeof(TWTL_DB_REGISTRY)*(structSize[3] + 1));
	nowHklmRunOnce = (TWTL_DB_REGISTRY*)realloc(nowHklmRunOnce, sizeof(TWTL_DB_REGISTRY)*(structSize[4] + 1));
	memset(nowHklmRunOnce, 0x00, sizeof(TWTL_DB_REGISTRY)*(structSize[4] + 1));
	nowServices = (TWTL_DB_SERVICE*)realloc(nowServices, sizeof(TWTL_DB_SERVICE)*(structSize[5] + 1));
	memset(nowServices, 0x00, sizeof(TWTL_DB_SERVICE)*(structSize[5] + 1));
	if (!SnapCurrentStatus(NULL, nowHkcuRun, nowHklmRun, nowHkcuRunOnce, nowHklmRunOnce, nowServices, NULL, NULL, NULL, 0))
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
		char* path = NULL;
		switch (type)
		{
		case REG_SHORT_HKCU_RUN:
			path = "/Reg/Short/UserRun/";
			break;
		case REG_SHORT_HKCU_RUNONCE:
			path = "/Reg/Short/UserRunOnce/";
			break;
		case REG_SHORT_HKLM_RUN:
			path = "/Reg/Short/GlobalRun/";
			break;
		case REG_SHORT_HKLM_RUNONCE:
			path = "/Reg/Short/GlobalRunOnce/";
			break;
		case REG_SHORT_SERVICE:
			path = "/Reg/Short/GlobalServices/";
			break;
		}

		json_t* contentStatus = json_object(); // [ { } , { } , { }, ] // {}
		json_array_append(contentsArray, contentStatus);
		json_object_set(contentStatus, "type", json_string(PROTO_STR_RES_STATUS));
		json_object_set(contentStatus, "path", json_string(path));
		json_object_set(contentStatus, "value", json_integer(PROTO_STATUS_SUCCESS));

		json_t* contentNode = json_object(); // [ { } , { } , { }, ] // {}
		json_array_append(contentsArray, contentNode);
		json_object_set(contentNode, "type", json_string(PROTO_STR_RES_OBJECT));
		json_object_set(contentNode, "path", json_string(path));
		

		// Services
		if (type == REG_SHORT_SERVICE)
		{ // ArraySize == structSize[5]
			json_t* serviceRoot = json_array();

			BOOL first = TRUE;

			// Find new entries
			for (DWORD idx_n = 0; idx_n < structSize[5]; idx_n++)
			{
				BOOL found = FALSE;
				for (int idx_l = 0; idx_l < countLastServices; idx_l++)
				{
					if (StrCmpW(nowServices[idx_n].key, lastServices[idx_l].key) == 0)
					{
						found = TRUE;
						break;
					}
				}

				if (found == FALSE)
				{ // Newly created!
					if (first)
					{
						json_object_set(contentNode, "value", serviceRoot);
						first = FALSE;
					}

					json_t* serviceNode = json_object();
					json_array_append(serviceRoot, serviceNode);

					{
						char utf8_buf[DB_MAX_REG_PATH];
						WideCharToMultiByte(CP_UTF8, 0, nowServices[idx_n].key, -1, utf8_buf, DB_MAX_REG_PATH, NULL, NULL);
						json_object_set(serviceNode, "Name", json_string(utf8_buf));
					}
					if (nowServices[idx_n].image_path[0] == L'\0')
					{
						json_object_set(serviceNode, "Value", json_null());
					}
					else
					{
						char utf8_buf[DB_MAX_REG_PATH];
						WideCharToMultiByte(CP_UTF8, 0, nowServices[idx_n].image_path, -1, utf8_buf, DB_MAX_REG_PATH, NULL, NULL);
						json_object_set(serviceNode, "Value", json_string(utf8_buf));
					}
				}
			}
		}
		else
		{
			int countLast = 0;
			DWORD countNow = 0;
			TWTL_DB_REGISTRY* dbLastReg = NULL;
			TWTL_DB_REGISTRY* dbNowReg = NULL;
			switch (type)
			{
			case REG_SHORT_HKCU_RUN:
				countLast = countLastHkcuRun;
				countNow = structSize[1];
				dbLastReg = lastHkcuRun;
				dbNowReg = nowHkcuRun;
				break;
			case REG_SHORT_HKCU_RUNONCE:
				countLast = countLastHkcuRunOnce;
				countNow = structSize[3];
				dbLastReg = lastHkcuRunOnce;
				dbNowReg = nowHkcuRunOnce;
				break;
			case REG_SHORT_HKLM_RUN:
				countLast = countLastHklmRun;
				countNow = structSize[2];
				dbLastReg = lastHklmRun;
				dbNowReg = nowHklmRun;
				break;
			case REG_SHORT_HKLM_RUNONCE:
				countLast = countLastHklmRunOnce;
				countNow = structSize[4];
				dbLastReg = lastHklmRunOnce;
				dbNowReg = nowHklmRunOnce;
				break;
			}

			json_t* regRoot = json_array();
			json_object_set(contentNode, "value", regRoot);
			// BOOL first = TRUE;
			// Find new entries
			for (DWORD idx_n = 0; idx_n < countNow; idx_n++)
			{
				BOOL found = FALSE;
				for (int idx_l = 0; idx_l < countLast; idx_l++)
				{
					if (StrCmpW(dbNowReg[idx_n].value, dbLastReg[idx_l].value) == 0)
					{
						found = TRUE;
						break;
					}
				}
				

				if (found == FALSE)
				{ // Newly created!
					/*
					if (first)
					{
						json_object_set(contentNode, "value", regRoot);
						first = FALSE;
					}
					*/

					json_t* regNode = json_object();
					json_array_append(regRoot, regNode);

					{
						char* utf8_buf = (char*)calloc(DB_MAX_REG_VALUE, sizeof(char));
						WideCharToMultiByte(CP_UTF8, 0, dbNowReg[idx_n].value, -1, utf8_buf, DB_MAX_REG_VALUE, NULL, NULL);
						json_object_set(regNode, "Name", json_string(utf8_buf));
						free(utf8_buf);
					}
					{
						char utf8_buf[DB_MAX_REG_NAME];
						WideCharToMultiByte(CP_UTF8, 0, dbNowReg[idx_n].name, -1, utf8_buf, DB_MAX_REG_NAME, NULL, NULL);
						json_object_set(regNode, "Value", json_string(utf8_buf));
					}
				}
			}
		}
	}

	// Finalize
	free(lastHkcuRun);
	free(lastHklmRun);
	free(lastHkcuRunOnce);
	free(lastHklmRunOnce);
	free(lastServices);
	free(nowHkcuRun);
	free(nowHklmRun);
	free(nowHkcuRunOnce);
	free(nowHklmRunOnce);
	free(nowServices);
	nowHkcuRun = NULL;
	nowHklmRun = NULL;
	nowHkcuRunOnce = NULL;
	nowHklmRunOnce = NULL;
	nowServices = NULL;
}

BOOL JSON_SendTrap(SOCKET sock, std::string path)
{
	json_t* root = json_object();
	json_object_set(root, "name", json_string(g_twtlInfo.engine.name.c_str()));
	json_object_set(root, "app", json_string(g_twtlInfo.engine.app.c_str()));
	json_object_set(root, "version", json_string(g_twtlInfo.engine.version.c_str()));
	json_t* contentsArray = json_array(); // "contents": [  ]  // []
	json_object_set(root, "contents", contentsArray);
	json_t* contentNode = json_object(); // [ { } , { } , { }, ] // {}
	json_array_append(contentsArray, contentNode);
	json_object_set(contentNode, "type", json_string(PROTO_STR_RES_STATUS));
	json_object_set(contentNode, "path", json_string(path.c_str()));
	json_object_set(contentNode, "Value", json_integer(PROTO_STATUS_SUCCESS));
	char* sendbuf = json_dumps(root, 0);
	json_decref(root);

#ifdef _DEBUG
	FILE* fp = NULL;
	fopen_s(&fp, "json_log.txt", "a");
	fprintf(fp, "[E -> U] Trap Send\n%s\n", sendbuf);
	fclose(fp);
#endif
	printf("[E -> U] Trap Send\n%s\n", sendbuf);

	int iResult = send(sock, sendbuf, strlen(sendbuf) + 1, 0);
	free(sendbuf);
	if (iResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(sock);
		WSACleanup();
		return TRUE;
	}
	return FALSE;
}

void JSON_Init_TWTL_INFO_DATA()
{
	JSON_Init_TWTL_INFO_ENGINE_NODE(&(g_twtlInfo.engine));
}


void JSON_Init_TWTL_INFO_ENGINE_NODE(TWTL_INFO_ENGINE_NODE* node)
{
	node->name = "TWTL";
	node->app = "TWTL-Engine";
	node->version = "1";
	node->reqPort = 5259;
	node->trapPort = INVALID_PORT_VALUE; // Invalid value
}


void JSON_Init_ProtoBufHeader(TWTL_PROTO_BUF* buf)
{
	buf->name = g_twtlInfo.engine.name;
	buf->app = g_twtlInfo.engine.app;
	buf->version = g_twtlInfo.engine.version;
}