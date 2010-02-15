#ifndef _XSCRIPT_STATE_H_
#define _XSCRIPT_STATE_H_

#include <map>
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <boost/cstdint.hpp>
#include <boost/utility.hpp>
#include <boost/lexical_cast.hpp>

namespace xscript {

class StateImpl;
class TypedValue;

class State : private boost::noncopyable {
public:
    State();
    virtual ~State();

    void clear();
    void erase(const std::string &key);
    void erasePrefix(const std::string &prefix);

    bool asBool(const std::string &name) const;
    void setBool(const std::string &name, bool value);

    boost::int32_t asLong(const std::string &name) const;
    void setLong(const std::string &name, boost::int32_t value);

    boost::int64_t asLongLong(const std::string &name) const;
    void setLongLong(const std::string &name, boost::int64_t value);

    boost::uint32_t asULong(const std::string &name) const;
    void setULong(const std::string &name, boost::uint32_t value);

    boost::uint64_t asULongLong(const std::string &name) const;
    void setULongLong(const std::string &name, boost::uint64_t value);

    double asDouble(const std::string &name) const;
    void setDouble(const std::string &name, double value);

    std::string asString(const std::string &name) const;
    std::string asString(const std::string &name, const std::string &default_value) const;
    void setString(const std::string &name, const std::string &value);

    template<typename Type> Type as(const std::string &name) const;
    template<typename Type> void set(const std::string &name, Type val);

    bool has(const std::string &name) const;
    void keys(std::vector<std::string> &v) const;

    void values(std::map<std::string, TypedValue> &v) const;
    void copy(const std::string &src, const std::string &dest);

    TypedValue typedValue(const std::string &name) const;
    TypedValue typedValue(const std::string &name, const TypedValue &default_value) const;

    void checkName(const std::string &name) const;

    /**
     * Check that state with name set to true
     */
    bool is(const std::string &name) const;
private:
    std::auto_ptr<StateImpl> impl_;
};

template<typename Type> inline Type
State::as(const std::string &name) const {
    return boost::lexical_cast<Type>(asString(name));
}

template<typename Type> inline void
State::set(const std::string &name, Type val) {
    setString(name, boost::lexical_cast<std::string>(val));
}

template<> inline bool
State::as<bool>(const std::string &name) const {
    return asBool(name);
}

template<> inline void
State::set<bool>(const std::string &name, bool val) {
    setBool(name, val);
}

template<> inline boost::int32_t
State::as<boost::int32_t>(const std::string &name) const {
    return asLong(name);
}

template<> inline void
State::set<boost::int32_t>(const std::string &name, boost::int32_t val) {
    setLong(name, val);
}

template<> inline boost::int64_t
State::as<boost::int64_t>(const std::string &name) const {
    return asLongLong(name);
}

template<> inline void
State::set<boost::int64_t>(const std::string &name, boost::int64_t val) {
    setLongLong(name, val);
}

template<> inline boost::uint32_t
State::as<boost::uint32_t>(const std::string &name) const {
    return asULong(name);
}

template<> inline void
State::set<boost::uint32_t>(const std::string &name, boost::uint32_t val) {
    setULong(name, val);
}

template<> inline boost::uint64_t
State::as<boost::uint64_t>(const std::string &name) const {
    return asULongLong(name);
}

template<> inline void
State::set<boost::uint64_t>(const std::string &name, boost::uint64_t val) {
    setULongLong(name, val);
}

template<> inline double
State::as<double>(const std::string &name) const {
    return asDouble(name);
}

template<> inline void
State::set<double>(const std::string &name, double val) {
    setDouble(name, val);
}

template<> inline std::string
State::as<std::string>(const std::string &name) const {
    return asString(name);
}

template<> inline void
State::set<const std::string&>(const std::string &name, const std::string &val) {
    setString(name, val);
}

} // namespace xscript

#endif // _XSCRIPT_STATE_H_
