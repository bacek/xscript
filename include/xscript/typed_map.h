#ifndef _XSCRIPT_TYPED_MAP_H_
#define _XSCRIPT_TYPED_MAP_H_

#include <map>
#include <string>
#include <vector>

#include <boost/cstdint.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>

#include "xscript/range.h"

namespace xscript {

class TypedValueVisitor {
public:
    virtual ~TypedValueVisitor() {}
    virtual void visitBool(bool) {}
    virtual void visitInt32(boost::int32_t) {};
    virtual void visitUInt32(boost::uint32_t) {};
    virtual void visitInt64(boost::int64_t) {};
    virtual void visitUInt64(boost::uint64_t) {};
    virtual void visitDouble(double) {};
    virtual void visitString(const std::string&) = 0;
    virtual void visitArray(const std::vector<std::string>&) = 0;
    virtual void visitMap(const std::map<std::string, std::string>&) = 0;
};

class ComplexTypedValue {
public:
    virtual ~ComplexTypedValue();
    virtual void visit(TypedValueVisitor *visitor) const = 0;
    virtual const std::string& stringType() const = 0;
    virtual std::string asString() const = 0;
    virtual void serialize(std::string &result) const = 0;
    virtual ComplexTypedValue* clone() const = 0;
};

class ArrayTypedValue : public ComplexTypedValue {
public:
    ArrayTypedValue(const std::vector<std::string> &value);
    ArrayTypedValue(const Range &value);
    virtual ~ArrayTypedValue();
    virtual void visit(TypedValueVisitor *visitor) const;
    virtual const std::string& stringType() const;
    virtual std::string asString() const;
    virtual void serialize(std::string &result) const;
    virtual ComplexTypedValue* clone() const;
private:
    static const std::string TYPE;
    std::vector<std::string> value_;
};

class MapTypedValue : public ComplexTypedValue {
public:
    MapTypedValue(const std::map<std::string, std::string> &value);
    MapTypedValue(const Range &value);
    virtual ~MapTypedValue();
    virtual void visit(TypedValueVisitor *visitor) const;
    virtual const std::string& stringType() const;
    virtual std::string asString() const;
    virtual void serialize(std::string &result) const;
    virtual ComplexTypedValue* clone() const;
private:
    static const std::string TYPE;
    std::map<std::string, std::string> value_;
};

class TypedMap;

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
    TypedValue(const std::vector<std::string> &value);
    TypedValue(const std::map<std::string, std::string> &value);
    TypedValue(unsigned int type, const Range &value);

    unsigned int type() const;
    const std::string& stringType() const;
    ComplexTypedValue* complexType() const;
    bool asBool() const;
    std::string asString() const;
    const std::string& simpleValue() const;
    void serialize(std::string &result) const;
    void visit(TypedValueVisitor *visitor, bool string_prefer) const;

    static const unsigned int TYPE_BOOL = 1;
    static const unsigned int TYPE_LONG = 1 << 1;
    static const unsigned int TYPE_ULONG = 1 << 2;
    static const unsigned int TYPE_LONGLONG = 1 << 3;
    static const unsigned int TYPE_ULONGLONG = 1 << 4;
    static const unsigned int TYPE_DOUBLE = 1 << 5;
    static const unsigned int TYPE_STRING = 1 << 6;
    static const unsigned int TYPE_ARRAY = 1 << 7;
    static const unsigned int TYPE_MAP = 1 << 8;

    static const std::string TYPE_BOOL_STRING;
    static const std::string TYPE_LONG_STRING;
    static const std::string TYPE_ULONG_STRING;
    static const std::string TYPE_LONGLONG_STRING;
    static const std::string TYPE_ULONGLONG_STRING;
    static const std::string TYPE_DOUBLE_STRING;
    static const std::string TYPE_STRING_STRING;

    friend class TypedMap;

private:
    unsigned int type_;
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

    std::string asString(const std::string &name) const;
    std::string asString(const std::string &name, const std::string &default_value) const;
    void setString(const std::string &name, const std::string &value);

    bool asBool(const std::string &name) const;
    void setBool(const std::string &name, bool value);

    bool has(const std::string &name) const;
    void keys(std::vector<std::string> &v) const;

    const std::map<std::string, TypedValue>& values() const;

    bool is(const std::string &name) const;

    TypedValue find(const std::string &name) const;
    bool find(const std::string &name, TypedValue &result) const;

    void set(const std::string &name, const TypedValue &value);

protected:
    typedef std::map<std::string, TypedValue> TypedValueMap;
    const TypedValue& getTypedValue(const std::string &name) const;
    template<typename T> T as(const std::string &name) const;

private:
    TypedValueMap values_;
};

template<typename T> inline T
TypedMap::as(const std::string &name) const {
    std::string value = getTypedValue(name).asString();
    if (value.empty()) {
        return static_cast<T>(0);
    }
    return boost::lexical_cast<T>(value);
}

} // namespace xscript

#endif // _XSCRIPT_TYPED_MAP_H_
