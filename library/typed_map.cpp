#include "settings.h"

#include <stdexcept>

#include "xscript/algorithm.h"
#include "xscript/string_utils.h"
#include "xscript/typed_map.h"

#include "internal/parser.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include "xscript/logger.h"

namespace xscript {

const std::string TypedValue::TYPE_NIL_STRING = "Nil";
const std::string TypedValue::TYPE_BOOL_STRING = "Boolean";
const std::string TypedValue::TYPE_LONG_STRING = "Long";
const std::string TypedValue::TYPE_ULONG_STRING = "ULong";
const std::string TypedValue::TYPE_LONGLONG_STRING = "LongLong";
const std::string TypedValue::TYPE_ULONGLONG_STRING = "ULongLong";
const std::string TypedValue::TYPE_DOUBLE_STRING = "Double";
const std::string TypedValue::TYPE_STRING_STRING = "String";

class TypedValue::ComplexTypedValue {
public:
    virtual ~ComplexTypedValue() {}
    virtual void visit(TypedValueVisitor *visitor) const = 0;
    virtual const std::string& stringType() const = 0;
    virtual const std::string& asString() const = 0;
    virtual boost::uint32_t serializedSize() const = 0;
    virtual void serialize(std::string &result) const = 0;
    virtual bool add(const std::string &key, const TypedValue &value) = 0;
    boost::uint32_t valueSerializedSize(const TypedValue &value) const {
        return value.serializedSize();
    }
};

class ArrayTypedValue : public TypedValue::ComplexTypedValue {
public:
    ArrayTypedValue();
    ArrayTypedValue(const TypedValue::ArrayType &value);
    ArrayTypedValue(const Range &value);
    virtual ~ArrayTypedValue();
    virtual void visit(TypedValueVisitor *visitor) const;
    virtual const std::string& stringType() const;
    virtual const std::string& asString() const;
    virtual boost::uint32_t serializedSize() const;
    virtual void serialize(std::string &result) const;
    virtual bool add(const std::string &key, const TypedValue &value);
private:
    static const std::string TYPE;
    TypedValue::ArrayType value_;
};

class MapTypedValue : public TypedValue::ComplexTypedValue {
public:
    MapTypedValue();
    MapTypedValue(const TypedValue::MapType &value);
    MapTypedValue(const Range &value);
    virtual ~MapTypedValue();
    virtual void visit(TypedValueVisitor *visitor) const;
    virtual const std::string& stringType() const;
    virtual const std::string& asString() const;
    virtual boost::uint32_t serializedSize() const;
    virtual void serialize(std::string &result) const;
    virtual bool add(const std::string &key, const TypedValue &value);
private:
    static const std::string TYPE;
    TypedValue::MapType value_;
};

TypedValue::TypedValue() : type_(TYPE_NIL)
{}

TypedValue::TypedValue(bool value) :
    type_(TYPE_BOOL), value_(value ? "1" : "0")
{} 
    
TypedValue::TypedValue(boost::int32_t value) :
    type_(TYPE_LONG), value_(boost::lexical_cast<std::string>(value))
{}

TypedValue::TypedValue(boost::uint32_t value) :
    type_(TYPE_ULONG), value_(boost::lexical_cast<std::string>(value))
{}

TypedValue::TypedValue(boost::int64_t value) :
    type_(TYPE_LONGLONG), value_(boost::lexical_cast<std::string>(value))
{}

TypedValue::TypedValue(boost::uint64_t value) :
    type_(TYPE_ULONGLONG), value_(boost::lexical_cast<std::string>(value))
{}

TypedValue::TypedValue(double value) :
    type_(TYPE_DOUBLE), value_(boost::lexical_cast<std::string>(value))
{}

TypedValue::TypedValue(const std::string &value) :
    type_(TYPE_STRING), value_(value)
{}

TypedValue::TypedValue(const ArrayType &value) :
    type_(TYPE_ARRAY), complex_(new ArrayTypedValue(value))
{}

TypedValue::TypedValue(const MapType &value) :
    type_(TYPE_MAP), complex_(new MapTypedValue(value))
{}

TypedValue::TypedValue(const Range &value) : type_(TYPE_NIL)
{
    const char* buf = value.begin();
    const char* end = value.end();
    if (end - buf < (boost::int64_t)(sizeof(boost::uint32_t) + sizeof(type_))) {
        throw std::runtime_error("Incorrect format of typed value");
    }

    boost::uint32_t size = *((boost::uint32_t*)buf);
    buf += sizeof(boost::uint32_t);
    size -= sizeof(boost::uint32_t);

    type_ = *((boost::uint16_t*)buf);
    buf += sizeof(type_);
    size -= sizeof(type_);

    if (end - buf < (boost::int64_t)size) {
        throw std::runtime_error("Incorrect format of typed value");
    }

    if (type_ >= TYPE_BOOL && type_ <= TYPE_STRING) {
        value_.assign(buf, size);
    }
    else if (type_ == TYPE_ARRAY) {
        complex_.reset(new ArrayTypedValue(Range(buf, buf + size)));
    }
    else if (type_ == TYPE_MAP) {
        complex_.reset(new MapTypedValue(Range(buf, buf + size)));
    }
    else {
        throw std::runtime_error("Unknown value type");
    }
}

TypedValue
TypedValue::createArrayValue() {
    TypedValue value;
    value.type_ = TYPE_ARRAY;
    value.complex_ = boost::shared_ptr<ComplexTypedValue>(new ArrayTypedValue());
    return value;
}

TypedValue
TypedValue::createMapValue() {
    TypedValue value;
    value.type_ = TYPE_MAP;
    value.complex_ = boost::shared_ptr<ComplexTypedValue>(new MapTypedValue());
    return value;
}

bool
TypedValue::add(const std::string &key, const TypedValue &value) {
    if (complex_.get()) {
        return complex_->add(key, value);
    }
    return false;
}

boost::uint16_t
TypedValue::type() const {
    return type_;
}

bool
TypedValue::nil() const {
    return TYPE_NIL == type_;
}

const std::string&
TypedValue::stringType() const {
    if (complex_.get()) {
        return complex_->stringType();
    }
    switch (type_) {
        case TYPE_NIL : return TYPE_NIL_STRING;
        case TYPE_BOOL : return TYPE_BOOL_STRING;
        case TYPE_LONG : return TYPE_LONG_STRING;
        case TYPE_ULONG : return TYPE_ULONG_STRING;
        case TYPE_LONGLONG : return TYPE_LONGLONG_STRING;
        case TYPE_ULONGLONG : return TYPE_ULONGLONG_STRING;
        case TYPE_DOUBLE : return TYPE_DOUBLE_STRING;
    };
    return TYPE_STRING_STRING;
}

bool
TypedValue::asBool() const {
    if (NULL != complex_.get()) {
        return true;
    }
    else if (trim(createRange(value_)).empty()) {
        return false;
    }
    else if (type_ == TypedValue::TYPE_STRING) {
        return true;
    }
    else if (type_ == TypedValue::TYPE_DOUBLE) {
        double val = boost::lexical_cast<double>(value_);
        if (val > std::numeric_limits<double>::epsilon() ||
            val < -std::numeric_limits<double>::epsilon()) {
                return true;
        }
        return false;
    }
    else {
        return value_ != "0";
    }
}

const std::string&
TypedValue::asString() const {
    return complex_.get() ? complex_->asString() : value_;
}

boost::uint32_t
TypedValue::serializedSize() const {
    return sizeof(boost::uint32_t) + sizeof(type_) +
        (complex_.get() ? complex_->serializedSize() : value_.size());
}

void
TypedValue::serialize(std::string &result) const {
    boost::uint32_t size = serializedSize();
    if (result.empty()) {
        result.reserve(serializedSize());
    }
    result.append((char*)&size, sizeof(size));
    result.append((char*)&type_, sizeof(type_));
    if (complex_.get()) {
        complex_->serialize(result);
    }
    else {
        result.append(value_);
    }
}

void
TypedValue::visit(TypedValueVisitor *visitor) const {
    if (NULL != complex_.get()) {
        complex_->visit(visitor);
        return;
    }
    switch (type_) {
    case TYPE_NIL:
        visitor->visitNil();
        break;
    case TYPE_BOOL:
        visitor->visitBool(boost::lexical_cast<bool>(value_));
        break;
    case TYPE_LONG:
        visitor->visitInt32(boost::lexical_cast<boost::int32_t>(value_));
        break;
    case TYPE_ULONG:
        visitor->visitUInt32(boost::lexical_cast<boost::uint32_t>(value_));
        break;
    case TYPE_LONGLONG:
        visitor->visitInt64(boost::lexical_cast<boost::int64_t>(value_));
        break;
    case TYPE_ULONGLONG:
        visitor->visitUInt64(boost::lexical_cast<boost::uint64_t>(value_));
        break;
    case TYPE_DOUBLE:
        visitor->visitDouble(boost::lexical_cast<double>(value_));
        break;
    case TYPE_STRING:
        visitor->visitString(value_);
    }
}

const TypedValue TypedMap::undefined_;

TypedMap::TypedMap() {
}

TypedMap::~TypedMap() {
}

void
TypedMap::clear() {
    values_.clear();
}

void
TypedMap::erase(const std::string &name) {
    values_.erase(name);
}

void
TypedMap::insert(const std::string &name, const TypedValue &value) {
    set(name, value);
}

boost::int32_t
TypedMap::asLong(const std::string &name) const {
    return as<boost::int32_t>(name);
}

void
TypedMap::setLong(const std::string &name, boost::int32_t value) {
    set(name, TypedValue(value));
}

boost::int64_t
TypedMap::asLongLong(const std::string &name) const {
    return as<boost::int64_t>(name);
}

void
TypedMap::setLongLong(const std::string &name, boost::int64_t value) {
    set(name, TypedValue(value));
}

boost::uint32_t
TypedMap::asULong(const std::string &name) const {
    return as<boost::uint32_t>(name);
}

void
TypedMap::setULong(const std::string &name, boost::uint32_t value) {
    set(name, TypedValue(value));
}

boost::uint64_t
TypedMap::asULongLong(const std::string &name) const {
    return as<boost::uint64_t>(name);
}

void
TypedMap::setULongLong(const std::string &name, boost::uint64_t value) {
    set(name, TypedValue(value));
}

double
TypedMap::asDouble(const std::string &name) const {
    return as<double>(name);
}

void
TypedMap::setDouble(const std::string &name, double value) {
    set(name, TypedValue(value));
}

const std::string&
TypedMap::asString(const std::string &name) const {
    return find(name).asString();
}

void
TypedMap::setString(const std::string &name, const std::string &value) {
    set(name, TypedValue(value));
}

void
TypedMap::erasePrefix(const std::string &prefix) {
    for (TypedValueMap::iterator i = values_.begin(), end = values_.end(); i != end; ) {
        if (strncmp(i->first.c_str(), prefix.c_str(), prefix.size()) == 0) {
            values_.erase(i++);
        }
        else {
            ++i;
        }
    }
}

bool
TypedMap::asBool(const std::string &name) const {
    return find(name).asBool();
}

void
TypedMap::setBool(const std::string &name, bool value) {
    set(name, TypedValue(value));
}

bool
TypedMap::has(const std::string &name) const {
    return values_.end() != values_.find(name);
}

void
TypedMap::keys(std::vector<std::string> &v) const {
    v.clear();
    v.reserve(values_.size());
    for (TypedValueMap::const_iterator i = values_.begin(), end = values_.end(); i != end; ++i) {
        v.push_back(i->first);
    }
}

const std::map<std::string, TypedValue>&
TypedMap::values() const {
    return values_;
}

const TypedValue&
TypedMap::find(const std::string &name) const {
    const TypedValue& value = findNoThrow(name);
    if (value.nil()) {
        throw std::invalid_argument("nonexistent typed value: " + name);
    }
    return value;
}

const TypedValue&
TypedMap::findNoThrow(const std::string &name) const {
    TypedValueMap::const_iterator it = values_.find(name);
    if (values_.end() == it) {
        return undefined_;
    }
    return it->second;
}

const std::string&
TypedMap::asString(const std::string &name, const std::string &default_value) const {
    TypedValueMap::const_iterator it = values_.find(name);
    if (values_.end() != it) {
        return it->second.asString();
    }
    return default_value;
}

bool
TypedMap::is(const std::string &name) const {
    TypedValueMap::const_iterator it = values_.find(name);
    if (values_.end() != it) {
        return it->second.asBool();
    }
    return false;
}

void
TypedMap::set(const std::string &name, const TypedValue &value) {
    values_[name] = value;
}


const std::string ArrayTypedValue::TYPE = "Array";

ArrayTypedValue::ArrayTypedValue()
{}

ArrayTypedValue::ArrayTypedValue(const TypedValue::ArrayType &value) : value_(value)
{}

ArrayTypedValue::ArrayTypedValue(const Range &value) {
    const char* buf = value.begin();
    const char* end = value.end();
    while (buf < end) {
        if (end - buf < (boost::int32_t)sizeof(boost::uint32_t)) {
            throw std::runtime_error("Incorrect format of array typed value");
        }
        boost::uint32_t size = *((boost::uint32_t*)buf);
        if (buf + size > end) {
            throw std::runtime_error("Incorrect format of array typed value");
        }
        TypedValue value(Range(buf, buf + size));
        value_.push_back(value);
        buf += size;
    }
}

ArrayTypedValue::~ArrayTypedValue()
{}

void
ArrayTypedValue::visit(TypedValueVisitor *visitor) const {
    visitor->visitArray(value_);
}

const std::string&
ArrayTypedValue::stringType() const {
    return TYPE;
}

const std::string&
ArrayTypedValue::asString() const {
    return value_.empty() ? StringUtils::EMPTY_STRING : value_[0].asString();
}

boost::uint32_t
ArrayTypedValue::serializedSize() const {
    boost::uint32_t size = 0;
    for (std::vector<TypedValue>::const_iterator it = value_.begin();
         it != value_.end();
         ++it) {
        size += valueSerializedSize(*it);
    }
    return size;
}

void
ArrayTypedValue::serialize(std::string &result) const {
    for (std::vector<TypedValue>::const_iterator it = value_.begin();
         it != value_.end();
         ++it) {
        it->serialize(result);
    }
}

bool
ArrayTypedValue::add(const std::string &key, const TypedValue &value) {
    (void)key;
    value_.push_back(value);
    return true;
}

const std::string MapTypedValue::TYPE = "Map";

MapTypedValue::MapTypedValue()
{}

MapTypedValue::MapTypedValue(const TypedValue::MapType &value) :
    value_(value)
{}

MapTypedValue::MapTypedValue(const Range &value) {
    const char* buf = value.begin();
    const char* end = value.end();
    while (buf < end) {
        if (end - buf < (boost::int32_t)sizeof(boost::uint32_t)) {
            throw std::runtime_error("Incorrect format of map typed value");
        }
        boost::uint32_t var = *((boost::uint32_t*)buf);
        buf += sizeof(boost::uint32_t);
        if (end - buf < (boost::int64_t)var) {
            throw std::runtime_error("Incorrect format of map typed value");
        }
        Range key(buf, buf + var);
        buf += var;
        if (end - buf < (boost::int32_t)sizeof(boost::uint32_t)) {
            throw std::runtime_error("Incorrect format of map typed value");
        }
        boost::uint32_t size = *((boost::uint32_t*)buf);
        if (buf + size > end) {
            throw std::runtime_error("Incorrect format of map typed value");
        }
        TypedValue value(Range(buf, buf + size));
        value_.push_back(std::make_pair(std::string(key.begin(), key.size()), value));
        buf += size;
    }
}

MapTypedValue::~MapTypedValue()
{}

void
MapTypedValue::visit(TypedValueVisitor *visitor) const {
    visitor->visitMap(value_);
}

const std::string&
MapTypedValue::stringType() const {
    return TYPE;
}

const std::string&
MapTypedValue::asString() const {
    return value_.empty() ? StringUtils::EMPTY_STRING : value_.begin()->second.asString();
}

boost::uint32_t
MapTypedValue::serializedSize() const {
    boost::uint32_t size = 0;
    for (std::vector<std::pair<std::string, TypedValue> >::const_iterator it = value_.begin();
         it != value_.end();
         ++it) {
        size += sizeof(boost::uint32_t) + it->first.size();
        size += valueSerializedSize(it->second);
    }
    return size;
}

void
MapTypedValue::serialize(std::string &result) const {
    for (std::vector<std::pair<std::string, TypedValue> >::const_iterator it = value_.begin();
         it != value_.end();
         ++it) {
        boost::uint32_t var = it->first.size();
        result.append((char*)&var, sizeof(var));
        result.append(it->first);
        it->second.serialize(result);
    }
}

bool
MapTypedValue::add(const std::string &key, const TypedValue &value) {
    if (!key.empty()) {
        value_.push_back(std::make_pair(key, value));
        return true;
    }
    return false;
}

} // namespace xscript
