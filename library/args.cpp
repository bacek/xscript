#include "settings.h"

#include <string.h>

#include <boost/lexical_cast.hpp>

#include "xscript/args.h"
#include "xscript/exception.h"
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

void
ArgList::addState(const Context *ctx) {
    (void)ctx;
    throw CriticalInvokeError("State param is not allowed in this context");
}

void
ArgList::addRequest(const Context *ctx) {
    (void)ctx;
    throw CriticalInvokeError("Request param is not allowed in this context");
}

void
ArgList::addRequestData(const Context *ctx) {
    (void)ctx;
    throw CriticalInvokeError("RequestData param is not allowed in this context");
}

void
ArgList::addTag(const TaggedBlock *tb, const Context *ctx) {
    (void)tb;
    (void)ctx;
    throw CriticalInvokeError("Tag param is not allowed in this context");
}

CommonArgList::CommonArgList() {
}

CommonArgList::~CommonArgList() {
}

void
CommonArgList::add(bool value) {
    args_.push_back(value ? "1" : "0");
}

void
CommonArgList::add(double value) {
    args_.push_back(boost::lexical_cast<std::string>(value));
}

void
CommonArgList::add(boost::int32_t value) {
    args_.push_back(boost::lexical_cast<std::string>(value));
}

void
CommonArgList::add(boost::int64_t value) {
    args_.push_back(boost::lexical_cast<std::string>(value));
}

void
CommonArgList::add(boost::uint32_t value) {
    args_.push_back(boost::lexical_cast<std::string>(value));
}

void
CommonArgList::add(boost::uint64_t value) {
    args_.push_back(boost::lexical_cast<std::string>(value));
}

void
CommonArgList::add(const std::string &value) {
    args_.push_back(boost::lexical_cast<std::string>(value));
}

bool
CommonArgList::empty() const {
    return args_.empty();
}

unsigned int
CommonArgList::size() const {
    return args_.size();
}

const std::string&
CommonArgList::at(unsigned int i) const {
    return args_.at(i);
}

const std::vector<std::string>&
CommonArgList::args() const {
    return args_;
}

} // namespace xscript
