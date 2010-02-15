#ifndef _XSCRIPT_TYPED_MAP_H_
#define _XSCRIPT_TYPED_MAP_H_

#include <map>
#include <string>
#include <vector>

#include <boost/cstdint.hpp>
#include <boost/lexical_cast.hpp>

namespace xscript {

class TypedValue {
public:
    TypedValue();
    TypedValue(bool value);
    TypedValue(boost::int32_t value);
    TypedValue(boost::uint32_t value);
    TypedValue(boost::int64_t value);
    TypedValue(boost::uint64_t value);
    TypedValue(double value);
    TypedValue(const std::string &value);
    TypedValue(int type, const std::string &value);

    inline int type() const {
        return type_;
    }

    inline const std::string& value() const {
        return value_;
    }

    const std::string& stringType() const;
    bool asBool() const;

    static const int TYPE_BOOL = 1;
    static const int TYPE_LONG = 1 << 1;
    static const int TYPE_ULONG = 1 << 2;
    static const int TYPE_LONGLONG = 1 << 3;
    static const int TYPE_ULONGLONG = 1 << 4;
    static const int TYPE_DOUBLE = 1 << 5;
    static const int TYPE_STRING = 1 << 6;

    static const std::string TYPE_BOOL_STRING;
    static const std::string TYPE_LONG_STRING;
    static const std::string TYPE_ULONG_STRING;
    static const std::string TYPE_LONGLONG_STRING;
    static const std::string TYPE_ULONGLONG_STRING;
    static const std::string TYPE_DOUBLE_STRING;
    static const std::string TYPE_STRING_STRING;

private:
    int type_;
    std::string value_;
};

class TypedMap {  
public:
    TypedMap();
    virtual ~TypedMap();

    void clear();
    void erase(const std::string &name);
    void erasePrefix(const std::string &prefix);
    void insert(const std::string &name, const TypedValue &value);
    
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

    bool asBool(const std::string &name) const;
    void setBool(const std::string &name, bool value);

    bool has(const std::string &name) const;
    void keys(std::vector<std::string> &v) const;

    void values(std::map<std::string, TypedValue> &v) const;

    bool is(const std::string &name) const;

    const TypedValue& find(const std::string &name) const;
    const TypedValue& find(const std::string &name, const TypedValue &default_value) const;
protected:

    void set(const std::string &name, const TypedValue &value);

    template<typename T> T as(const std::string &name) const;

private:
    typedef std::map<std::string, TypedValue> TypedValueMap;
    TypedValueMap values_;
};

template<typename T> inline T
TypedMap::as(const std::string &name) const {
    const TypedValue &val = find(name);
    if (val.value().empty()) {
        return static_cast<T>(0);
    }
    return boost::lexical_cast<T>(val.value());
}

} // namespace xscript

#endif // _XSCRIPT_TYPED_MAP_H_
