#pragma once

#include "Log/Log.h"

#define FIND_AND_ASSERT_(it, map, key, mapname, keyname, printed_keyvalue) \
	auto it = map.find(key); \
	if (it == map.end()) { LOGEVENTL("ERROR", _ln("File") << __FILE__ << _ln("Line") << __LINE__ << _ln(keyname) << printed_keyvalue << " not found in " << mapname) } \
	else

#define FIND_AND_ASSERT(it, map, key, mapname, keyname) FIND_AND_ASSERT_(it, map, key, mapname, keyname, key)