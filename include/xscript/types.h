#ifndef _XSCRIPT_TYPES_H_
#define _XSCRIPT_TYPES_H_

#if defined(HAVE_STLPORT_HASHMAP)
#include <hash_set>
#include <hash_map>
#elif defined(HAVE_EXT_HASH_MAP) || defined(HAVE_GNUCXX_HASHMAP)
#include <ext/hash_set>
#include <ext/hash_map>
#else
#include <map>
#endif

#include <set>

#include "xscript/functors.h"

namespace xscript {

#if defined(HAVE_GNUCXX_HASHMAP)

typedef __gnu_cxx::hash_map<std::string, std::string, StringCIHash, StringCIEqual> HeaderMap;
typedef __gnu_cxx::hash_map<std::string, std::string, StringCIHash> VarMap;

#elif defined(HAVE_EXT_HASH_MAP) || defined(HAVE_STLPORT_HASHMAP)

typedef std::hash_map<std::string, std::string, StringCIHash, StringCIEqual> HeaderMap;
typedef std::hash_map<std::string, std::string, StringCIHash> VarMap;

#else

typedef std::map<std::string, std::string, StringCILess> HeaderMap;
typedef std::map<std::string, std::string> VarMap;

#endif

typedef HeaderMap CookieMap;
typedef std::set<Cookie, CookieLess> CookieSet;

} // namespace xscript

#endif // _XSCRIPT_TYPES_H_
