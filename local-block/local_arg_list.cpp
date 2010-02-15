#include "settings.h"

#include <stdexcept>

#include <xscript/context.h>
#include <xscript/exception.h>

#include "local_arg_list.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

LocalArgList::LocalArgList()
{}

LocalArgList::~LocalArgList() {
}

void
LocalArgList::add(bool value) {
    value_ = TypedValue(value);
}

void
LocalArgList::add(double value) {
    value_ = TypedValue(value);
}

void
LocalArgList::add(boost::int32_t value) {
    value_ = TypedValue(value);
}

void
LocalArgList::add(boost::int64_t value) {
    value_ = TypedValue(value);
}

void
LocalArgList::add(boost::uint32_t value) {
    value_ = TypedValue(value);
}

void
LocalArgList::add(boost::uint64_t value) {
    value_ = TypedValue(value);
}

void
LocalArgList::add(const std::string &value) {
    value_ = TypedValue(value);
}

void
LocalArgList::addAs(const std::string &type, const TypedValue &value) {
    if (type.empty()) {
        value_ = value;
    }
    else {
        ArgList::addAs(type, value);
    }
}

void
LocalArgList::addState(const Context *ctx) {
    (void)ctx;
    throw CriticalInvokeError("State param is not allowed in this context");
}

void
LocalArgList::addRequest(const Context *ctx) {
    (void)ctx;
    throw CriticalInvokeError("Request param is not allowed in this context");
}

void
LocalArgList::addRequestData(const Context *ctx) {
    (void)ctx;
    throw CriticalInvokeError("RequestData param is not allowed in this context");
}

void
LocalArgList::addTag(const TaggedBlock *tb, const Context *ctx) {
    (void)tb;
    (void)ctx;
    throw CriticalInvokeError("Tag param is not allowed in this context");
}

const TypedValue&
LocalArgList::value() const {
    return value_;
}

} // namespace xscript
