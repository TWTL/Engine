#pragma once

#ifdef TWTL_JSON_EXPORTS
#define TWTL_JSON_API extern "C" __declspec(dllexport) 
#else
#define TWTL_JSON_API extern "C" __declspec(dllimport) 
#endif


#define TWTL_JSON_MAX_BUF	2048
#define TWTL_JSON_PORT		5259
#define TWTL_JSON_PORT_WSTR	L"5259"

