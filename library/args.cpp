#include "settings.h"

#include <string.h>
#include <map>

#include <boost/lexical_cast.hpp>

#include "xscript/args.h"
#include "xscript/context.h"
#include "xscript/exception.h"
#include "xscript/functors.h"
#include "xscript/logger.h"
#include "xscript/operation_mode.h"
#include "xscript/state.h"
#include "xscript/typed_map.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

typedef void (*args_appender)(ArgList& args, const std::string &type, const std::string &value, bool suppress_error);

inline static void
processCastError(const std::string &type, const std::string &value, bool checked) {
    if (checked) {
        throw std::runtime_error("bad cast to '" + type + "' : " + value);
    }
}

static void
addBool(ArgList& args, const std::string &type, const std::string &value, bool checked) {
    bool b = false;
    if ("1" == value || !strncasecmp(value.c_str(), "true", sizeof("true"))) {
        b = true;
    }
    else if (value.empty() || "0" == value || !strncasecmp(value.c_str(), "false", sizeof("false"))) {
        b = false;
    }
    else {
        processCastError(type, value, checked);
    }
    args.add(b);
}

static void
addDouble(ArgList& args, const std::string &type, const std::string &value, bool checked) {
    double d = 0;
    if (!value.empty()) {
        try {
            d = boost::lexical_cast<double>(value);
        }
        catch (const boost::bad_lexical_cast &) {
            processCastError(type, value, checked);
        }
    }
    args.add(d);
}

template <typename T>
static void
addNumeric(ArgList& args, const std::string &type, const std::string &value, bool checked) {
    T d = 0;
    if (!value.empty()) {
        try {
            d = boost::lexical_cast<T>(value);
        }
        catch (const boost::bad_lexical_cast &) {
            processCastError(type, value, checked);

            //try convert to double and cast to native type
            try {
                double dd = boost::lexical_cast<double>(value);
                d = static_cast<T>(dd);
            }
            catch (const boost::bad_lexical_cast &) {
            }
        }
    }
    args.add(d);
}

typedef std::map< std::string, args_appender, CILess<std::string> > ArgAppenders;
static ArgAppenders map_;

struct ArgsAppenderRegisterer {
    ArgsAppenderRegisterer() {
        map_["bool"] = &addBool;
        map_["boolean"] = &addBool;
        map_["float"] = &addDouble;
        map_["double"] = &addDouble;
        map_["long"] = &addNumeric<boost::int32_t>;
        map_["ulong"] = &addNumeric<boost::uint32_t>;
        map_["unsigned long"] = &addNumeric<boost::uint32_t>;
        map_["longlong"] = &addNumeric<boost::int64_t>;
        map_["long long"] = &addNumeric<boost::int64_t>;
        map_["ulonglong"] = &addNumeric<boost::uint64_t>;
        map_["unsigned long long"] = &addNumeric<boost::uint64_t>;
    }
};

static ArgsAppenderRegisterer args_appender_registerer_;

static const std::string STR_TRUE("1");
static const std::string STR_FALSE("0");

ArgList::ArgList() {
}

ArgList::~ArgList() {
}

void
ArgList::addAsChecked(const std::string &type, const std::string &value, bool checked) {

    //log()->debug("converting %s to type '%s', value: %s", checked ? "" : "(checked)", type.c_str(), value.c_str());
    if (type.empty() || !strncasecmp(type.c_str(), "string", sizeof("string"))) {
        add(value);
	return;
    }

    const ArgAppenders::const_iterator it = map_.find(type);
    if (it != map_.end()) {
        args_appender f = it->second;
        assert(f);
        (*f)(*this, type, value, checked);
	return;
    }

    std::string error_msg = "bad cast to strange type '" + type + "' value: " + value;
    if (checked || !OperationMode::instance()->isProduction()) {
        throw CriticalInvokeError(error_msg);
    }
    log()->warn("%s (add as string)", error_msg.c_str());
    add(value);
}

void
ArgList::addAs(const std::string &type, const std::string &value) {
    addAsChecked(type, value, false);
}

void
ArgList::addAs(const std::string &type, const TypedValue &value) {
    addAs(type.empty() ? value.stringType() : type, value.asString());
}

void
ArgList::addState(const Context *ctx) {
    add(ctx->state()->asString());
}

void
ArgList::addRequest(const Context *ctx) {
    add(ctx->request()->getQueryString());
}

void
ArgList::addRequestData(const Context *ctx) {
    add(ctx->request()->getQueryString());
}

void
ArgList::addTag(const TaggedBlock *tb, const Context *ctx) {
    (void)tb;
    (void)ctx;
    throw CriticalInvokeError("Tag param is not allowed in this context");
}


StringArgList::StringArgList() {
}

StringArgList::~StringArgList() {
}

void
StringArgList::add(bool value) {
    args_.push_back(value ? STR_TRUE : STR_FALSE);
}

void
StringArgList::add(double value) {
    args_.push_back(boost::lexical_cast<std::string>(value));
}

void
StringArgList::add(boost::int32_t value) {
    args_.push_back(boost::lexical_cast<std::string>(value));
}

void
StringArgList::add(boost::int64_t value) {
    args_.push_back(boost::lexical_cast<std::string>(value));
}

void
StringArgList::add(boost::uint32_t value) {
    args_.push_back(boost::lexical_cast<std::string>(value));
}

void
StringArgList::add(boost::uint64_t value) {
    args_.push_back(boost::lexical_cast<std::string>(value));
}

void
StringArgList::add(const std::string &value) {
    args_.push_back(value);
}

bool
StringArgList::empty() const {
    return args_.empty();
}

unsigned int
StringArgList::size() const {
    return args_.size();
}

const std::string&
StringArgList::at(unsigned int i) const {
    return args_.at(i);
}

const std::vector<std::string>&
StringArgList::args() const {
    return args_;
}


CheckedStringArgList::CheckedStringArgList(bool checked) : checked_(checked) {
}

CheckedStringArgList::~CheckedStringArgList() {
}

void
CheckedStringArgList::addAs(const std::string &type, const std::string &value) {
    addAsChecked(type, value, checked_);
}


CommonArgList::CommonArgList() {
}

CommonArgList::~CommonArgList() {
}

void
CommonArgList::add(bool value) {
    args_.push_back(value ? STR_TRUE : STR_FALSE);
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
    args_.push_back(value);
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
