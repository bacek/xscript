#include "settings.h"

#include <cstring>
#include <limits>
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
    data_.erasePrefix(prefix);
}

bool
StateImpl::has(const std::string &name) const {
    boost::mutex::scoped_lock sl(mutex_);
    return data_.has(name);
}

void
StateImpl::keys(std::vector<std::string> &v) const {
    std::vector<std::string> local;
    {
        boost::mutex::scoped_lock sl(mutex_);
        data_.keys(local);
    }
    v.swap(local);
}

void
StateImpl::values(std::map<std::string, TypedValue> &v) const {
    boost::mutex::scoped_lock sl(mutex_);
    data_.values(v);
}

void
StateImpl::copy(const std::string &src, const std::string &dest) {
    boost::mutex::scoped_lock sl(mutex_);
    const TypedValue &val = data_.find(src);
    data_.insert(dest, val);
}

std::string
StateImpl::asString(const std::string &name, const std::string &default_value) const {
    boost::mutex::scoped_lock sl(mutex_);
    return data_.asString(name, default_value);
}

bool
StateImpl::is(const std::string &name) const {
    boost::mutex::scoped_lock sl(mutex_);
    return data_.is(name);
}

} // namespace xscript
