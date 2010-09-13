#include "settings.h"

#include <cstring>
#include <sstream>
#include <stdexcept>

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread/mutex.hpp>

#include "xscript/state.h"
#include "xscript/logger.h"
#include "details/state_impl.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

State::State() :
        impl_(new StateImpl()) {
}

State::~State() {
}

void
State::clear() {
    impl_->clear();
}

void
State::erase(const std::string &key) {
    impl_->erase(key);
}

void
State::erasePrefix(const std::string &prefix) {
    impl_->erasePrefix(prefix);
}

bool
State::asBool(const std::string &name) const {
    return impl_->asBool(name);
}

void
State::setBool(const std::string &name, bool value) {
    impl_->setBool(name, value);
}

boost::int32_t
State::asLong(const std::string &name) const {
    return impl_->asLong(name);
}

void
State::setLong(const std::string &name, boost::int32_t value) {
    impl_->setLong(name, value);
}

boost::int64_t
State::asLongLong(const std::string &name) const {
    return impl_->asLongLong(name);
}

void
State::setLongLong(const std::string &name, boost::int64_t value) {
    impl_->setLongLong(name, value);
}

boost::uint32_t
State::asULong(const std::string &name) const {
    return impl_->asULong(name);
}

void
State::setULong(const std::string &name, boost::uint32_t value) {
    impl_->setULong(name, value);
}

boost::uint64_t
State::asULongLong(const std::string &name) const {
    return impl_->asULongLong(name);
}

void
State::setULongLong(const std::string &name, boost::uint64_t value) {
    impl_->setULongLong(name, value);
}

double
State::asDouble(const std::string &name) const {
    return impl_->asDouble(name);
}

void
State::setDouble(const std::string &name, double value) {
    impl_->setDouble(name, value);
}

std::string
State::asString(const std::string &name) const {
    return impl_->asString(name);
}

std::string
State::asString(const std::string &name, const std::string &default_value) const {
    return impl_->asString(name, default_value);
}

void
State::setString(const std::string &name, const std::string &value) {
    impl_->setString(name, value);
}

void
State::setTypedValue(const std::string &name, const TypedValue &value) {
    impl_->set(name, value);
}

bool
State::has(const std::string &name) const {
    return impl_->has(name);
}

void
State::keys(std::vector<std::string> &v) const {
    impl_->keys(v);
}

void
State::values(std::map<std::string, TypedValue> &v) const {
    impl_->values(v);
}

void
State::values(const std::string &prefix, std::map<std::string, TypedValue> &v) const {
    impl_->values(prefix, v);
}

void
State::copy(const std::string &src, const std::string &dest) {
    impl_->copy(src, dest);
}

TypedValue
State::typedValue(const std::string &name) const {
    return impl_->typedValue(name);
}

void
State::checkName(const std::string &name) const {
    if (name.empty()) {
        throw std::invalid_argument("empty state name");
    }
}

bool
State::is(const std::string &name) const {
    return impl_->is(name);
}

std::string
State::asString() const {
    std::stringstream stream;
    std::map<std::string, TypedValue> vals;
    values(vals);
    for(std::map<std::string, TypedValue>::iterator i = vals.begin(), end = vals.end();
        i != end;
        ++i) {
        stream << i->first << ":" << i->second.asString() << std::endl;
    }
    return stream.str();
}

} // namespace xscript
