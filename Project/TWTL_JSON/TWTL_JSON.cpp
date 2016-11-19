// TWTL_JSON.cpp : DLL 응용 프로그램을 위해 내보낸 함수를 정의합니다.
//

#include "stdafx.h"

#include "TWTL_JSON.h"

/*
* Parse text into a JSON object. If text is valid JSON, returns a
* json_t structure, otherwise prints and error and returns null.
*/
#define MAX_CHARS 4096

DWORD JSON_Parse(const char buf[], size_t buflen) {
	/* parse text into JSON structure */
	json_t *root;
	json_error_t error;
	TWTL_PROTO_BUF req;
	memset(&req, 0, sizeof(TWTL_PROTO_BUF));

	root = json_loadb(buf, buflen, 0, &error);

	if (!root) { // Not valid JSON text
		fprintf(stderr, "json error on line %d: %s\n", error.line, error.text);
		return TRUE;
	}

	JSON_ProtoParse(root, &req, 0);
	json_decref(root);



	JSON_ProtoClearNode(&req);

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

void JSON_ProtoParse(json_t *element, TWTL_PROTO_BUF* req, int depth) {
	switch (json_typeof(element)) {
	case JSON_OBJECT:
		size_t size;
		const char *key;
		json_t *value;

		size = json_object_size(element);

		printf("JSON Object of %ld pair\n", size);
		json_object_foreach(element, key, value) {
			if (StrCmpIA(key, "content") == 0)
			{
				JSON_ProtoAddNode(req);
			}
			JSON_ProtoParse(value, req, depth + 1);
		}
		break;
	case JSON_ARRAY:
		size_t i;
		size_t size = json_array_size(element);
		const char *key;
		json_t *value;

		printf("JSON Array of %ld element\n", size);
		for (i = 0; i < size; i++) {
			value = json_array_get(element, i);
			JSON_ProtoParse(value, req, depth + 1);
		}
		break;
	case JSON_STRING:
		if (depth == 1)
		{
			if (StrCmpIA(key, "name") == 0)
				StringCchCopyA(req->name, TWTL_PROTO_MAX_BUF, json_string_value(value));
			else if (stricmp(key, "app") == 0)
				StringCchCopyA(req->app, TWTL_PROTO_MAX_BUF, json_string_value(value));
			else if (stricmp(key, "version") == 0)
				StringCchCopyA(req->version, TWTL_PROTO_MAX_BUF, json_string_value(value));
		}
		else if (depth == 2)
		{
			TWTL_PROTO_NODE* node = JSON_ProtoAddNode(req);
			if (StrCmpIA(key, "type") == 0)
			{
				if (StrCmpIA(json_string_value(value), "requset.get") == 0)
					node->type = PROTO_REQ_GET;
				else if (StrCmpIA(json_string_value(value), "requset.set") == 0)
					node->type = PROTO_REQ_SET;
				else if (StrCmpIA(json_string_value(value), "response.status") == 0)
					node->type = PROTO_RES_STATUS;
				else if (StrCmpIA(json_string_value(value), "response.object") == 0)
					node->type = PROTO_RES_OBJECT;
			}
			else if (stricmp(key, "name") == 0)
				StringCchCopyA(node->name, TWTL_PROTO_MAX_BUF, json_string_value(value));
			else if (stricmp(key, "value") == 0)
			{
				node->value_type = PROTO_VALUE_STR;
				StringCchCopyA(node->value_str, TWTL_PROTO_MAX_BUF, json_string_value(value));
			}
		}
		break;
	case JSON_INTEGER:
		if (depth == 2 && stricmp(key, "value") == 0)
		{
			TWTL_PROTO_NODE* node = JSON_ProtoAddNode(req);
			node->value_type = PROTO_VALUE_INT;
			node->value_int = json_integer_value(value);
		}
		break;
	case JSON_REAL:
		if (depth == 2 && stricmp(key, "value") == 0)
		{
			TWTL_PROTO_NODE* node = JSON_ProtoAddNode(req);
			node->value_type = PROTO_VALUE_REAL;
			node->value_real = json_real_value(value);
		}
		break;
	case JSON_TRUE:
		if (depth == 2 && stricmp(key, "value") == 0)
		{
			TWTL_PROTO_NODE* node = JSON_ProtoAddNode(req);
			node->value_type = PROTO_VALUE_BOOL;
			node->value_bool = TRUE;
		}
		break;
	case JSON_FALSE:
		if (depth == 2 && stricmp(key, "value") == 0)
		{
			TWTL_PROTO_NODE* node = JSON_ProtoAddNode(req);
			node->value_type = PROTO_VALUE_BOOL;
			node->value_bool = FALSE;
		}
		break;
	case JSON_NULL:
		if (depth == 2 && stricmp(key, "value") == 0)
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
	while (req_node)
	{
		switch (req->type)
		{
		case PROTO_REQ_GET:
			break;
		case PROTO_REQ_SET:
			break;
		case PROTO_RES_STATUS:
			break;
		case PROTO_RES_OBJECT:
			break;
		}
		if (StrCmpIA(req_node->name, "") == 0)
		req_node = req_node->next;
	}

	memset(req_node, 0, sizeof(TWTL_PROTO_NODE));


	TWTL_PROTO_NODE* res_node = JSON_ProtoAddNode(res);
	// if (res_node->type)
}
