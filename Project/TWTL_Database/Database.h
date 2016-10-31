#pragma once

#include "stdafx.h"

enum DB_TABLE_TYPE 
{ 
	DB_PROCESS,
	DB_HKLM_RUN, DB_HKLM_RUNONCE, DB_HKCU_RUN, DB_HKCU_RUNONCE,
	DB_NETWORK
};