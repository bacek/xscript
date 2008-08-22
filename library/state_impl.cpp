#include "settings.h"

#include <cstring>
#include <limits>
#include <sstream>
#include <iterator>
#include <stdexcept>

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread/mutex.hpp>

#include "xscript/algorithm.h"
#include "xscript/range.h"
#include "xscript/logger.h"
#include "details/state_impl.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

StateImpl::StateImpl() {
}

StateImpl::~StateImpl() {
}

void
StateImpl::erasePrefix(const std::string &prefix) {

    boost::mutex::scoped_lock sl(mutex_);
    for (StateValueMap::iterator i = values_.begin(), end = values_.end(); i != end; ) {
        if (strncmp(i->first.c_str(), prefix.c_str(), prefix.size()) == 0) {
            values_.erase(i++);
        }
        else {
            ++i;
        }
    }
}

bool
StateImpl::asBool(const std::string &name) const {
    boost::mutex::scoped_lock sl(mutex_);
    return asBoolInternal(name);
}

bool
StateImpl::asBoolInternal(const std::string &name) const {
    const StateValue &val = find(name);

    if (trim(createRange(val.value())).empty()) {
        return false;
    }
    else if (val.type() == StateValue::TYPE_STRING) {
        return true;
    }
    else if (val.type() == StateValue::TYPE_DOUBLE) {
        double value = boost::lexical_cast<double>(val.value());
        if (value > std::numeric_limits<double>::epsilon() ||
            value < -std::numeric_limits<double>::epsilon()) {
                return true;
        }
        return false;
    }
    else {
        return val.value() != "0";
    }
}

void
StateImpl::setBool(const std::string &name, bool value) {
    boost::mutex::scoped_lock sl(mutex_);
    std::string v = value ? "1" : "0";
    set(name, StateValue::TYPE_BOOL, v);
}

bool
StateImpl::has(const std::string &name) const {
    boost::mutex::scoped_lock sl(mutex_);
    return hasInternal(name);
}

bool
StateImpl::hasInternal(const std::string &name) const {
    StateValueMap::const_iterator i = values_.find(name);
    return (values_.end() != i);
}

void
StateImpl::keys(std::vector<std::string> &v) const {

    std::vector<std::string> local;
    boost::mutex::scoped_lock sl(mutex_);
    for (StateValueMap::const_iterator i = values_.begin(), end = values_.end(); i != end; ++i) {
        local.push_back(i->first);
    }
    v.swap(local);
}

void
StateImpl::values(std::map<std::string, StateValue> &v) const {
    std::map<std::string, StateValue> local;
    boost::mutex::scoped_lock sl(mutex_);
    std::copy(values_.begin(), values_.end(), std::inserter(local, local.begin()));
    local.swap(v);
}

void
StateImpl::copy(const std::string &src, const std::string &dest) {
    boost::mutex::scoped_lock sl(mutex_);
    const StateValue &val = find(src);
    set(dest, val.type(), val.value());
}

const StateValue&
StateImpl::find(const std::string &name) const {
    StateValueMap::const_iterator i = values_.find(name);
    if (values_.end() != i) {
        return i->second;
    }
    else {
        std::stringstream stream;
        stream << "nonexistent state param " << name;
        throw std::invalid_argument(stream.str());
    }
}

bool
StateImpl::is(const std::string &name) const {
    boost::mutex::scoped_lock sl(mutex_);
    return hasInternal(name) && asBoolInternal(name);
}

} // namespace xscript
