#ifndef _XSCRIPT_INTERNAL_HASH_H_
#define _XSCRIPT_INTERNAL_HASH_H_

#include "settings.h"

#if defined(HAVE_GNUCXX_HASHMAP) || defined(HAVE_EXT_HASH_MAP)|| defined(HAVE_STLPORT_HASHMAP) 

#include <string>
#include <functional>

namespace xscript
{
namespace details
{

struct StringHash : public std::unary_function<const std::string&, std::size_t>
{
	std::size_t operator () (const std::string &str) const {
		std::size_t value = 0;
		for (std::string::const_iterator i = str.begin(), end = str.end(); i != end; ++i) {
			value += 5 * (*i);
		}
		return value;
	}
};

} // namespace details
} // namespace xscript

#endif // defined(HAVE_GNUCXX_HASHMAP) || defined(HAVE_EXT_HASH_MAP)|| defined(HAVE_STLPORT_HASHMAP) 

#endif // _XSCRIPT_INTERNAL_HASH_H_
