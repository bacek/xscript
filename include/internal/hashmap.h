#ifndef _XSCRIPT_INTERNAL_HASH_MAP_H_
#define _XSCRIPT_INTERNAL_HASH_MAP_H_

#include "settings.h"

#if defined(HAVE_STLPORT_HASHMAP)
#define HAVE_HASHMAP 1
#include <hash_map>
#elif defined(HAVE_EXT_HASH_MAP) || defined(HAVE_GNUCXX_HASHMAP)
#define HAVE_HASHMAP 1
#include <ext/hash_map>
#endif

namespace xscript {
namespace details {

#if defined (HAVE_GNUCXX_HASHMAP)
using __gnu_cxx::hash_map;
#elif defined(HAVE_EXT_HASH_MAP) || defined(HAVE_STLPORT_HASHMAP)
using std::hash_map;
#endif

} // namespace details
} // namespace xscript

#endif // _XSCRIPT_INTERNAL_HASH_MAP_H_
