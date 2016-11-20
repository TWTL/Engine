// TWTL_JSON.cpp : DLL 응용 프로그램을 위해 내보낸 함수를 정의합니다.
//

#include "stdafx.h"

#include "TWTL_JSON.h"

/*
* Parse text into a JSON object. If text is valid JSON, returns a
* json_t structure, otherwise prints and error and returns null.
*/
#define MAX_CHARS 4096

static TWTL_INFO_DATA g_twtlInfo;

// Must call JSON_ClearNode outside
DWORD JSON_Parse(const char buf[], size_t buflen, TWTL_PROTO_BUF* req)
{
	/* parse text into JSON structure */
	json_t *root;
	json_error_t error;
	memset(req, 0, sizeof(TWTL_PROTO_BUF));

	// root = json_loads(buf, 0, &error);
	root = json_loadb(buf, buflen, 0, &error);

	if (!root) { // Not valid JSON text
		fprintf(stderr, "json error on line %d: %s\n", error.line, error.text);
		return TRUE;
	}

	JSON_ProtoParse(root, "root", req, 0);
	json_decref(root);

	return FALSE;
}

/*
{
    "glossary": {
        "title": "example glossary",
        "GlossDiv": {
			"title": "S",
			"GlossList": {
				"GlossEntry": {
					"ID": "SGML",
					"SortAs": "SGML",
					"GlossTerm": "Standard Generalized Markup Language",
					"Acronym": "SGML",
					"Abbrev": "ISO 8879:1986",
					"GlossDef": {
						"para": "A meta-markup language, used to create markup languages such as DocBook.",
						"GlossSeeAlso": [
							"GML", "XML"]
					},
					"GlossSee": 123
                }
            }
        }
    }
}

JSON Object of 1 pair:
  JSON Key: "glossary"
  JSON Object of 2 pairs:
    JSON Key: "title"
    JSON String: "example glossary"
    JSON Key: "GlossDiv"
    JSON Object of 2 pairs:
      JSON Key: "title"
      JSON String: "S"
      JSON Key: "GlossList"
      JSON Object of 1 pair:
        JSON Key: "GlossEntry"
        JSON Object of 7 pairs:
          JSON Key: "ID"
          JSON String: "SGML"
          JSON Key: "SortAs"
          JSON String: "SGML"
          JSON Key: "GlossTerm"
          JSON String: "Standard Generalized Markup Language"
          JSON Key: "Acronym"
          JSON String: "SGML"
          JSON Key: "Abbrev"
          JSON String: "ISO 8879:1986"
          JSON Key: "GlossDef"
          JSON Object of 2 pairs:
            JSON Key: "para"
            JSON String: "A meta-markup language, used to create markup languages such as DocBook."
            JSON Key: "GlossSeeAlso"
            JSON Array of 2 elements:
              JSON String: "GML"
              JSON String: "XML"
          JSON Key: "GlossSee"
          JSON Integer: "123"
Connection closing...
*/

TWTL_PROTO_NODE* JSON_ProtoAddNode(TWTL_PROTO_BUF* req)
{
	TWTL_PROTO_NODE* now = req->contents;
	while (now)
	{
		now = now->next;
	}

	now = (TWTL_PROTO_NODE*) malloc(sizeof(TWTL_PROTO_NODE));
	memset(now, 0, sizeof(TWTL_PROTO_NODE));

	return now;
}

void JSON_ProtoClearNode(TWTL_PROTO_BUF* req)
{
	TWTL_PROTO_NODE* now = req->contents;
	while (now)
	{
		TWTL_PROTO_NODE* next = now->next;
		free(next);
		now = next;
	}
	free(req);

	return;
}

void JSON_ProtoParse(json_t *element, const char *key, TWTL_PROTO_BUF* req, int depth)
{
	size_t i;
	size_t size;
	const char *foundKey;
	json_t *value;

	switch (json_typeof(element)) {
	case JSON_OBJECT:
		size = json_object_size(element);

		printf("JSON Object of %ld pair\n", size);
		json_object_foreach(element, foundKey, value) {
			if (StrCmpIA(key, "content") == 0)
			{
				JSON_ProtoAddNode(req);
			}
			JSON_ProtoParse(value, foundKey, req, depth + 1);
		}
		break;
	case JSON_ARRAY:
		size = json_array_size(element);

		printf("JSON Array of %ld element\n", size);
		for (i = 0; i < size; i++) {
			value = json_array_get(element, i);
			JSON_ProtoParse(value, "array", req, depth + 1);
		}
		break;
	case JSON_STRING:
		if (depth == 1)
		{
			if (StrCmpIA(key, "name") == 0)
				StringCchCopyA(req->name, TWTL_PROTO_MAX_BUF, json_string_value(element));
			else if (StrCmpIA(key, "app") == 0)
				StringCchCopyA(req->app, TWTL_PROTO_MAX_BUF, json_string_value(element));
			else if (StrCmpIA(key, "version") == 0)
				StringCchCopyA(req->version, TWTL_PROTO_MAX_BUF, json_string_value(element));
		}
		else if (depth == 2)
		{
			TWTL_PROTO_NODE* node = JSON_ProtoAddNode(req);
			if (StrCmpIA(key, "type") == 0)
			{
				if (StrCmpIA(json_string_value(element), "requset.get") == 0)
					node->type = PROTO_REQ_GET;
				else if (StrCmpIA(json_string_value(element), "requset.set") == 0)
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
		}
		break;
	case JSON_INTEGER:
		if (depth == 2 && StrCmpIA(key, "value") == 0)
		{
			TWTL_PROTO_NODE* node = JSON_ProtoAddNode(req);
			node->value_type = PROTO_VALUE_INT;
			node->value_int = json_integer_value(element);
		}
		break;
	case JSON_REAL:
		if (depth == 2 && StrCmpIA(key, "value") == 0)
		{
			TWTL_PROTO_NODE* node = JSON_ProtoAddNode(req);
			node->value_type = PROTO_VALUE_REAL;
			node->value_real = json_real_value(element);
		}
		break;
	case JSON_TRUE:
		if (depth == 2 && StrCmpIA(key, "value") == 0)
		{
			TWTL_PROTO_NODE* node = JSON_ProtoAddNode(req);
			node->value_type = PROTO_VALUE_BOOL;
			node->value_bool = TRUE;
		}
		break;
	case JSON_FALSE:
		if (depth == 2 && StrCmpIA(key, "value") == 0)
		{
			TWTL_PROTO_NODE* node = JSON_ProtoAddNode(req);
			node->value_type = PROTO_VALUE_BOOL;
			node->value_bool = FALSE;
		}
		break;
	case JSON_NULL:
		if (depth == 2 && StrCmpIA(key, "value") == 0)
		{
			TWTL_PROTO_NODE* node = JSON_ProtoAddNode(req);
			node->value_type = PROTO_VALUE_NULL;
		}
		break;
	default:
		fprintf(stderr, "unrecognized JSON type %d\n", json_typeof(element));
	}
}

void JSON_ProtoParsePath(TWTL_PROTO_BUF* req, TWTL_PROTO_BUF* res)
{

}

void JSON_ProtoMakeResponse(TWTL_PROTO_BUF* req, TWTL_PROTO_BUF* res)
{
	TWTL_PROTO_NODE* req_node = req->contents;
	memset(res, 0, sizeof(TWTL_PROTO_BUF));
	while (req_node)
	{
		switch (req_node->type)
		{
		case PROTO_REQ_GET:
			if (StrCmpIA(req_node->path, "/Engine/Name/") == 0)
			{
				TWTL_PROTO_NODE* res_node_status = JSON_ProtoAddNode(res);
				res_node_status->type = PROTO_RES_STATUS;
				res_node_status->value_type = PROTO_VALUE_INT;
				res_node_status->value_int = PROTO_STATUS_SUCCESS;

				TWTL_PROTO_NODE* res_node_object = JSON_ProtoAddNode(res);
				res_node_object->type = PROTO_RES_OBJECT;
				res_node_object->value_type = PROTO_VALUE_STR;
				StringCchCopyA(res_node_object->value_str, TWTL_PROTO_MAX_BUF, g_twtlInfo.engine.version);
			}
			else if (StrCmpIA(req_node->path, "/Engine/Version/") == 0)
			{
				TWTL_PROTO_NODE* res_node_status = JSON_ProtoAddNode(res);
				res_node_status->type = PROTO_RES_STATUS;
				res_node_status->value_type = PROTO_VALUE_INT;
				res_node_status->value_int = PROTO_STATUS_SUCCESS;

				TWTL_PROTO_NODE* res_node_object = JSON_ProtoAddNode(res);
				res_node_object->type = PROTO_RES_OBJECT;
				res_node_object->value_type = PROTO_VALUE_STR;
				StringCchCopyA(res_node_object->value_str, TWTL_PROTO_MAX_BUF, g_twtlInfo.engine.name);
			}
			else if (StrCmpIA(req_node->path, "/Engine/RequsetPort/") == 0)
			{
				TWTL_PROTO_NODE* res_node_status = JSON_ProtoAddNode(res);
				res_node_status->type = PROTO_RES_STATUS;
				res_node_status->value_type = PROTO_VALUE_INT;
				res_node_status->value_int = PROTO_STATUS_SUCCESS;

				TWTL_PROTO_NODE* res_node_object = JSON_ProtoAddNode(res);
				res_node_object->type = PROTO_RES_OBJECT;
				res_node_object->value_type = PROTO_VALUE_INT;
				res_node_object->value_int = g_twtlInfo.engine.reqPort;
			}
			else if (StrCmpIA(req_node->path, "/Engine/TrapPort/") == 0)
			{ // This value must be set first
				if (g_twtlInfo.engine.trapPort == INVALID_PORT_VALUE)
				{ // Not set, return 400
					TWTL_PROTO_NODE* res_node_status = JSON_ProtoAddNode(res);
					res_node_status->type = PROTO_RES_STATUS;
					res_node_status->value_type = PROTO_VALUE_INT;
					res_node_status->value_int = PROTO_STATUS_CLIENT_ERROR;
				}
				else
				{ // Return 200
					TWTL_PROTO_NODE* res_node_status = JSON_ProtoAddNode(res);
					res_node_status->type = PROTO_RES_STATUS;
					res_node_status->value_type = PROTO_VALUE_INT;
					res_node_status->value_int = PROTO_STATUS_SUCCESS;

					TWTL_PROTO_NODE* res_node_object = JSON_ProtoAddNode(res);
					res_node_object->type = PROTO_RES_OBJECT;
					res_node_object->value_type = PROTO_VALUE_INT;
					res_node_object->value_int = g_twtlInfo.engine.trapPort;
				}				
			}
			break;
		case PROTO_REQ_SET:
			if (StrCmpIA(req_node->path, "/Engine/Name/") == 0)
			{ // Const value, produce error
				TWTL_PROTO_NODE* res_node_status = JSON_ProtoAddNode(res);
				res_node_status->type = PROTO_RES_STATUS;
				res_node_status->value_type = PROTO_VALUE_INT;
				res_node_status->value_int = PROTO_STATUS_CLIENT_ERROR;
			}
			else if (StrCmpIA(req_node->path, "/Engine/Version/") == 0)
			{ // Const value, produce error
				TWTL_PROTO_NODE* res_node_status = JSON_ProtoAddNode(res);
				res_node_status->type = PROTO_RES_STATUS;
				res_node_status->value_type = PROTO_VALUE_INT;
				res_node_status->value_int = PROTO_STATUS_CLIENT_ERROR;
			}
			else if (StrCmpIA(req_node->path, "/Engine/RequsetPort/") == 0)
			{ // Const value, produce error
				TWTL_PROTO_NODE* res_node_status = JSON_ProtoAddNode(res);
				res_node_status->type = PROTO_RES_STATUS;
				res_node_status->value_type = PROTO_VALUE_INT;
				res_node_status->value_int = PROTO_STATUS_CLIENT_ERROR;
			}
			else if (StrCmpIA(req_node->path, "/Engine/TrapPort/") == 0)
			{ // 
				g_twtlInfo.engine.trapPort = (SHORT) req_node->value_int;

				TWTL_PROTO_NODE* res_node_status = JSON_ProtoAddNode(res);
				res_node_status->type = PROTO_RES_STATUS;
				res_node_status->value_type = PROTO_VALUE_INT;
				res_node_status->value_int = PROTO_STATUS_SUCCESS;
			}
			break;
		}

		req_node = req_node->next;
	}

	memset(req_node, 0, sizeof(TWTL_PROTO_NODE));

	TWTL_PROTO_NODE* res_node = JSON_ProtoAddNode(res);
	// if (res_node->type)
}

json_t* JSON_ProtoBufToJson(TWTL_PROTO_BUF* res)
{
	json_t* root = json_object();
	json_t* json_arr = json_array();

	TWTL_PROTO_NODE* res_node = res->contents;

	json_object_set_new(root, "name", json_string(res->name));
	json_object_set_new(root, "app", json_string(res->app));
	json_object_set_new(root, "version", json_string(res->version));
	json_object_set_new(root, "contents", json_arr);

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
			json_object_set_new(json_node, "type", json_string(res_node->value_str));
			break;
		case PROTO_VALUE_INT:
			json_object_set_new(json_node, "type", json_integer(res_node->value_int));
			break;
		case PROTO_VALUE_REAL:
			json_object_set_new(json_node, "type", json_real(res_node->value_real));
			break;
		case PROTO_VALUE_BOOL:
			json_object_set_new(json_node, "type", json_boolean(res_node->value_bool));
			break;
		case PROTO_VALUE_NULL:
			break;
		}

		json_array_append(json_arr, json_node);

		res_node = res_node->next;
	}

	return root;
}


TWTL_JSON_API void JSON_Init_TWTL_INFO_DATA()
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

