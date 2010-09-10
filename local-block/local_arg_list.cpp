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
LocalArgList::addAs(const std::string &type, const TypedValue &value) {
    type.empty() ? add(value) : ArgList::addAs(type, value);
}

void
LocalArgList::add(bool value) {
    args_.push_back(TypedValue(value));
}

void
LocalArgList::add(double value) {
    args_.push_back(TypedValue(value));
}

void
LocalArgList::add(boost::int32_t value) {
    args_.push_back(TypedValue(value));
}

void
LocalArgList::add(boost::int64_t value) {
    args_.push_back(TypedValue(value));
}

void
LocalArgList::add(boost::uint32_t value) {
    args_.push_back(TypedValue(value));
}

void
LocalArgList::add(boost::uint64_t value) {
    args_.push_back(TypedValue(value));
}

void
LocalArgList::add(const std::string &value) {
    args_.push_back(TypedValue(value));
}

void
LocalArgList::add(const TypedValue &value) {
    args_.push_back(value);
}

bool
LocalArgList::empty() const {
    return args_.empty();
}

unsigned int
LocalArgList::size() const {
    return args_.size();
}

const std::string&
LocalArgList::at(unsigned int i) const {
    return args_.at(i).asString();
}

const TypedValue&
LocalArgList::typedValue(unsigned int i) const {
    return args_.at(i);
}

} // namespace xscript
