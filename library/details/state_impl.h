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

#include "xscript/typed_map.h"

namespace xscript {

class StateImpl : private boost::noncopyable {
public:
    StateImpl();
    virtual ~StateImpl();

    inline void clear() {
        boost::mutex::scoped_lock sl(mutex_);
        data_.clear();
    }

    inline void erase(const std::string &key) {
        boost::mutex::scoped_lock sl(mutex_);
        data_.erase(key);
    }

    void erasePrefix(const std::string &prefix);

    inline boost::int32_t asLong(const std::string &name) const {
        boost::mutex::scoped_lock sl(mutex_);
        return data_.asLong(name);
    }

    inline void setLong(const std::string &name, boost::int32_t value) {
        boost::mutex::scoped_lock sl(mutex_);
        data_.setLong(name, value);
    }

    inline boost::int64_t asLongLong(const std::string &name) const {
        boost::mutex::scoped_lock sl(mutex_);
        return data_.asLongLong(name);
    }

    inline void setLongLong(const std::string &name, boost::int64_t value) {
        boost::mutex::scoped_lock sl(mutex_);
        data_.setLongLong(name, value);
    }

    inline boost::uint32_t asULong(const std::string &name) const {
        boost::mutex::scoped_lock sl(mutex_);
        return data_.asULong(name);
    }

    inline void setULong(const std::string &name, boost::uint32_t value) {
        boost::mutex::scoped_lock sl(mutex_);
        data_.setULong(name, value);
    }

    inline boost::uint64_t asULongLong(const std::string &name) const {
        boost::mutex::scoped_lock sl(mutex_);
        return data_.asULongLong(name);
    }

    inline void setULongLong(const std::string &name, boost::uint64_t value) {
        boost::mutex::scoped_lock sl(mutex_);
        data_.setULongLong(name, value);
    }

    inline double asDouble(const std::string &name) const {
        boost::mutex::scoped_lock sl(mutex_);
        return data_.asDouble(name);
    }

    inline void setDouble(const std::string &name, double value) {
        boost::mutex::scoped_lock sl(mutex_);
        data_.setDouble(name, value);
    }

    inline std::string asString(const std::string &name) const {
        boost::mutex::scoped_lock sl(mutex_);
        return data_.asString(name);
    }

    std::string asString(const std::string &name, const std::string &default_value) const;
    
    inline void setString(const std::string &name, const std::string &value) {
        boost::mutex::scoped_lock sl(mutex_);
        data_.setString(name, value);
    }

    inline bool asBool(const std::string &name) const {
        boost::mutex::scoped_lock sl(mutex_);
        return data_.asBool(name);
    }
    
    inline void setBool(const std::string &name, bool value) {
        boost::mutex::scoped_lock sl(mutex_);
        data_.setBool(name, value);
    }

    bool has(const std::string &name) const;
    void keys(std::vector<std::string> &v) const;

    void values(std::map<std::string, TypedValue> &v) const;
    void copy(const std::string &src, const std::string &dest);

    TypedValue typedValue(const std::string &name) const;
    TypedValue typedValue(const std::string &name, const TypedValue &default_value) const;

    bool is(const std::string &name) const;

private:
    TypedMap data_;
    mutable boost::mutex mutex_;
};

} // namespace xscript

#endif // _XSCRIPT_STATE_IMPL_H_
