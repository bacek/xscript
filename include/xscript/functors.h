#ifndef _XSCRIPT_FUNCTORS_H_
#define _XSCRIPT_FUNCTORS_H_

#include <string>
#include <cctype>
#include <cstddef>
#include <algorithm>
#include <functional>
#include <boost/bind.hpp>

#include "xscript/range.h"
#include "xscript/cookie.h"

namespace xscript {

template<typename Cont>
struct CILess : public std::binary_function<const Cont&, const Cont&, bool> {
    bool comp(char val, char target) const {
        return tolower(val) < tolower(target);
    }

    bool operator() (const Cont &val, const Cont &target) const {
        return std::lexicographical_compare(val.begin(), val.end(), target.begin(), target.end(),
                                            boost::bind(&CILess::comp, this, _1, _2));
    }
};

struct CookieLess : public std::binary_function<const Cookie&, const Cookie&, bool> {
    inline bool operator() (const Cookie &cookie, const Cookie &target) const {
        return cookie.name() < target.name();
    }
};

typedef CILess<Range> RangeCILess;

template<>
struct CILess<std::string> {

    bool operator() (const std::string &val, const std::string &target) const {
	return strcasecmp(val.c_str(), target.c_str()) < 0;
    }
};

#if defined(HAVE_GNUCXX_HASHMAP) || defined(HAVE_EXT_HASH_MAP) || defined(HAVE_STLPORT_HASHMAP)

struct StringCIHash : public std::unary_function<const std::string&, std::size_t> {
    std::size_t operator () (const std::string &str) const {
        std::size_t value = 0;
        for (std::string::const_iterator i = str.begin(), end = str.end(); i != end; ++i) {
            value += 5 * tolower(*i);
        }
        return value;
    }
};

struct StringCIEqual : public std::binary_function<const std::string&, const std::string&, bool> {
    bool operator () (const std::string& str, const std::string& target) const {
        if (str.size() == target.size()) {
            return strncasecmp(str.c_str(), target.c_str(), str.size()) == 0;
        }
        return false;
    }
};

#else

typedef CILess<std::string> StringCILess;

#endif

} // namespace xscript

#endif // _XSCRIPT_FUNCTORS_H_
