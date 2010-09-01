#ifndef _XSCRIPT_TYPED_MAP_H_
#define _XSCRIPT_TYPED_MAP_H_

#include <map>
#include <string>
#include <vector>

#include <boost/cstdint.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>

#include "xscript/range.h"
#include "xscript/string_utils.h"

namespace xscript {

class TypedMap;
class TypedValueVisitor;

class TypedValue {
public:
    typedef std::vector<TypedValue> ArrayType;
    typedef std::vector<std::pair<std::string, TypedValue> > MapType;

    TypedValue();
    TypedValue(bool value);
    TypedValue(boost::int32_t value);
    TypedValue(boost::uint32_t value);
    TypedValue(boost::int64_t value);
    TypedValue(boost::uint64_t value);
    TypedValue(double value);
    TypedValue(const std::string &value);
    TypedValue(const ArrayType &value);
    TypedValue(const MapType &value);
    TypedValue(const Range &value);

    static TypedValue createArrayValue();
    static TypedValue createMapValue();

    bool add(const std::string &key, const TypedValue &value);

    boost::uint16_t type() const;
    bool nil() const;
    const std::string& stringType() const;
    bool asBool() const;
    const std::string& asString() const;
    void serialize(std::string &result) const;
    void visit(TypedValueVisitor *visitor) const;

    static const unsigned int TYPE_NIL = 0;
    static const unsigned int TYPE_BOOL = 1;
    static const unsigned int TYPE_LONG = 1 << 1;
    static const unsigned int TYPE_ULONG = 1 << 2;
    static const unsigned int TYPE_LONGLONG = 1 << 3;
    static const unsigned int TYPE_ULONGLONG = 1 << 4;
    static const unsigned int TYPE_DOUBLE = 1 << 5;
    static const unsigned int TYPE_STRING = 1 << 6;
    static const unsigned int TYPE_ARRAY = 1 << 7;
    static const unsigned int TYPE_MAP = 1 << 8;

    static const std::string TYPE_NIL_STRING;
    static const std::string TYPE_BOOL_STRING;
    static const std::string TYPE_LONG_STRING;
    static const std::string TYPE_ULONG_STRING;
    static const std::string TYPE_LONGLONG_STRING;
    static const std::string TYPE_ULONGLONG_STRING;
    static const std::string TYPE_DOUBLE_STRING;
    static const std::string TYPE_STRING_STRING;

    class ComplexTypedValue;
    friend class ComplexTypedValue;

private:
    boost::uint32_t serializedSize() const;
    friend class TypedMap;

private:
    boost::uint16_t type_;
    std::string value_;

    boost::shared_ptr<ComplexTypedValue> complex_;
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

    const std::string& asString(const std::string &name) const;
    const std::string& asString(const std::string &name, const std::string &default_value) const;
    void setString(const std::string &name, const std::string &value);

    bool asBool(const std::string &name) const;
    void setBool(const std::string &name, bool value);

    bool has(const std::string &name) const;
    void keys(std::vector<std::string> &v) const;

    const std::map<std::string, TypedValue>& values() const;
    void values(const std::string &prefix, std::map<std::string, TypedValue> &values) const;

    bool is(const std::string &name) const;

    const TypedValue& find(const std::string &name) const;
    const TypedValue& findNoThrow(const std::string &name) const;

    void set(const std::string &name, const TypedValue &value);

protected:
    typedef std::map<std::string, TypedValue> TypedValueMap;
    template<typename T> T as(const std::string &name) const;

private:
    static const TypedValue undefined_;
    TypedValueMap values_;
};

template<typename T> inline T
TypedMap::as(const std::string &name) const {
    const std::string& value = find(name).asString();
    if (value.empty()) {
        return static_cast<T>(0);
    }
    return boost::lexical_cast<T>(value);
}

class TypedValueVisitor {
public:
    virtual ~TypedValueVisitor() {}
    virtual void visitNil() {}
    virtual void visitBool(bool) {}
    virtual void visitInt32(boost::int32_t) {};
    virtual void visitUInt32(boost::uint32_t) {};
    virtual void visitInt64(boost::int64_t) {};
    virtual void visitUInt64(boost::uint64_t) {};
    virtual void visitDouble(double) {};
    virtual void visitString(const std::string&) = 0;
    virtual void visitArray(const TypedValue::ArrayType&) = 0;
    virtual void visitMap(const TypedValue::MapType&) = 0;
};

} // namespace xscript

#endif // _XSCRIPT_TYPED_MAP_H_
