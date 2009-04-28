#ifndef _XSCRIPT_STATE_IMPL_H_
#define _XSCRIPT_STATE_IMPL_H_

#include <map>
#include <string>
#include <vector>
#include <functional>
#include <boost/cstdint.hpp>
#include <boost/utility.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread/mutex.hpp>

#include "internal/hash.h"
#include "internal/hashmap.h"
#include "xscript/state_value.h"

namespace xscript {

#ifndef HAVE_HASHMAP
typedef std::map<std::string, StateValue> StateValueMap;
#else
typedef details::hash_map<std::string, StateValue, details::StringHash> StateValueMap;
#endif

class StateImpl : private boost::noncopyable {
public:
    StateImpl();
    virtual ~StateImpl();

    inline void clear() {
        boost::mutex::scoped_lock sl(mutex_);
        values_.clear();
    }

    inline void erase(const std::string &key) {
        boost::mutex::scoped_lock sl(mutex_);
        values_.erase(key);
    }

    void erasePrefix(const std::string &prefix);

    inline boost::int32_t asLong(const std::string &name) const {
        boost::mutex::scoped_lock sl(mutex_);
        return as<boost::int32_t>(name);
    }

    inline void setLong(const std::string &name, boost::int32_t value) {
        boost::mutex::scoped_lock sl(mutex_);
        set(name, StateValue::TYPE_LONG, boost::lexical_cast<std::string>(value));
    }

    inline boost::int64_t asLongLong(const std::string &name) const {
        boost::mutex::scoped_lock sl(mutex_);
        return as<boost::int64_t>(name);
    }

    inline void setLongLong(const std::string &name, boost::int64_t value) {
        boost::mutex::scoped_lock sl(mutex_);
        set(name, StateValue::TYPE_LONGLONG, boost::lexical_cast<std::string>(value));
    }

    inline boost::uint32_t asULong(const std::string &name) const {
        boost::mutex::scoped_lock sl(mutex_);
        return as<boost::uint32_t>(name);
    }

    inline void setULong(const std::string &name, boost::uint32_t value) {
        boost::mutex::scoped_lock sl(mutex_);
        set(name, StateValue::TYPE_ULONG, boost::lexical_cast<std::string>(value));
    }

    inline boost::uint64_t asULongLong(const std::string &name) const {
        boost::mutex::scoped_lock sl(mutex_);
        return as<boost::uint64_t>(name);
    }

    inline void setULongLong(const std::string &name, boost::uint64_t value) {
        boost::mutex::scoped_lock sl(mutex_);
        set(name, StateValue::TYPE_ULONGLONG, boost::lexical_cast<std::string>(value));
    }

    inline double asDouble(const std::string &name) const {
        boost::mutex::scoped_lock sl(mutex_);
        return as<double>(name);
    }

    inline void setDouble(const std::string &name, double value) {
        boost::mutex::scoped_lock sl(mutex_);
        set(name, StateValue::TYPE_DOUBLE, boost::lexical_cast<std::string>(value));
    }

    inline std::string asString(const std::string &name) const {
        boost::mutex::scoped_lock sl(mutex_);
        return find(name).value();
    }

    std::string asString(const std::string &name, const std::string &default_value) const;
    
    inline void setString(const std::string &name, const std::string &value) {
        boost::mutex::scoped_lock sl(mutex_);
        set(name, StateValue::TYPE_STRING, value);
    }

    bool asBool(const std::string &name) const;
    void setBool(const std::string &name, bool value);

    bool has(const std::string &name) const;
    void keys(std::vector<std::string> &v) const;

    void values(std::map<std::string, StateValue> &m) const;
    void copy(const std::string &src, const std::string &dest);

    void checkName(const std::string &name) const;

    inline StateValue typedValue(const std::string& name) const {
        boost::mutex::scoped_lock sl(mutex_);
        return find(name);
    }

    bool is(const std::string &name) const;

protected:
    const StateValue& find(const std::string &name) const;

    inline void set(const std::string &name, int type, const std::string &value) {
        values_[name] = StateValue(type, value);
    }

    template<typename T> T as(const std::string &name) const;

private:
    StateValueMap values_;
    mutable boost::mutex mutex_;
};

template<typename T> inline T
StateImpl::as(const std::string &name) const {
    const StateValue &val = find(name);
    if (val.value().empty()) {
        return static_cast<T>(0);
    }
    return boost::lexical_cast<T>(val.value());
}

} // namespace xscript

#endif // _XSCRIPT_STATE_IMPL_H_
