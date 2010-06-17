#include "settings.h"

#include <string.h>

#include <boost/lexical_cast.hpp>

#include "xscript/args.h"
#include "xscript/typed_map.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

ArgList::ArgList() {
}

ArgList::~ArgList() {
}

void
ArgList::addAs(const std::string &type, const std::string &value) {
    if (strncasecmp(type.c_str(), "bool", sizeof("bool")) == 0 ||
        strncasecmp(type.c_str(), "boolean", sizeof("boolean")) == 0) {
        bool b = false;
        if ("1" == value || strncasecmp(value.c_str(), "true", sizeof("true")) == 0) {
            b = true;
        }
        add(b);
    }
    else if (strncasecmp(type.c_str(), "double", sizeof("double")) == 0 ||
             strncasecmp(type.c_str(), "float", sizeof("float")) == 0) {
        double d = 0.0;
        try {
            d = boost::lexical_cast<double>(value);
        }
        catch (const boost::bad_lexical_cast &e) {
        }
        add(d);
    }
    else if (strncasecmp(type.c_str(), "long", sizeof("long")) == 0) {
        boost::int32_t l = 0;
        try {
            l = boost::lexical_cast<boost::int32_t>(value);
        }
        catch (const boost::bad_lexical_cast &e) {
        }
        add(l);
    }
    else if (strncasecmp(type.c_str(), "ulong", sizeof("ulong")) == 0 ||
             strncasecmp(type.c_str(), "unsigned long", sizeof("unsigned long")) == 0) {
        boost::uint32_t ul = 0;
        try {
            ul = boost::lexical_cast<boost::uint32_t>(value);
        }
        catch (const boost::bad_lexical_cast &e) {
        }
        add(ul);
    }
    else if (strncasecmp(type.c_str(), "longlong", sizeof("longlong")) == 0 ||
             strncasecmp(type.c_str(), "long long", sizeof("long long")) == 0) {
        boost::int64_t ll = 0;
        try {
            ll = boost::lexical_cast<boost::int64_t>(value);
        }
        catch (const boost::bad_lexical_cast &e) {
        }
        add(ll);
    }
    else if (strncasecmp(type.c_str(), "ulonglong", sizeof("ulonglong")) == 0 ||
             strncasecmp(type.c_str(), "unsigned long long", sizeof("unsigned long long")) == 0) {
        boost::uint64_t ull = 0;
        try {
            ull = boost::lexical_cast<boost::uint64_t>(value);
        }
        catch (const boost::bad_lexical_cast &e) {
        }
        add(ull);
    }
    else {
        add(value);
    }
}

void
ArgList::addAs(const std::string &type, const TypedValue &value) {
    addAs(type.empty() ? value.stringType() : type, value.asString());
}

} // namespace xscript
