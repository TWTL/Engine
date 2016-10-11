#pragma once

#ifdef TWTL_DATABASE_EXPORTS
#define TWTL_DATABASE_API __declspec(dllexport) 
#else
#define TWTL_DATABASE_API __declspec(dllimport) 
#endif

