#include "settings.h"

#include <stdexcept>

#include "xscript/algorithm.h"
#include "xscript/string_utils.h"
#include "xscript/typed_map.h"

#include "internal/parser.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

const std::string TypedValue::TYPE_BOOL_STRING = "Boolean";
const std::string TypedValue::TYPE_LONG_STRING = "Long";
const std::string TypedValue::TYPE_ULONG_STRING = "ULong";
const std::string TypedValue::TYPE_LONGLONG_STRING = "LongLong";
const std::string TypedValue::TYPE_ULONGLONG_STRING = "ULongLong";
const std::string TypedValue::TYPE_DOUBLE_STRING = "Double";
const std::string TypedValue::TYPE_STRING_STRING = "String";

TypedValue::TypedValue() : type_(TYPE_UNDEFINED)
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

TypedValue::TypedValue(const std::vector<std::string> &value) :
    type_(TYPE_ARRAY), complex_(new ArrayTypedValue(value))
{}

TypedValue::TypedValue(const std::vector<StringUtils::NamedValue> &value) :
    type_(TYPE_MAP), complex_(new MapTypedValue(value))
{}

TypedValue::TypedValue(unsigned int type, const Range &value) :
    type_(type)
{
    if (type >= TYPE_BOOL && type <= TYPE_STRING) {
        const char* buf = value.begin();
        const char* end = value.end();
        if (end - buf < (boost::int32_t)sizeof(boost::uint32_t)) {
            throw std::runtime_error("Incorrect format of typed value");
        }
        boost::uint32_t var = *((boost::uint32_t*)buf);
        buf += sizeof(boost::uint32_t);
        if (end - buf < (boost::int64_t)var) {
            throw std::runtime_error("Incorrect format of typed value");
        }
        value_.assign(buf, var);
    }
    else if (type == TYPE_ARRAY) {
        complex_.reset(new ArrayTypedValue(value));
    }
    else if (type == TYPE_MAP) {
        complex_.reset(new MapTypedValue(value));
    }
    else {
        throw std::runtime_error("Unknown value type");
    }
}

unsigned int
TypedValue::type() const {
    return type_;
}

bool
TypedValue::undefined() const {
    return TYPE_UNDEFINED == type_;
}

const std::string&
TypedValue::stringType() const {
    if (complex_.get()) {
        return complex_->stringType();
    }
    else if (type_ == TYPE_BOOL) {
        return TYPE_BOOL_STRING;
    }
    else if (type_ == TYPE_LONG) {
        return TYPE_LONG_STRING;
    }
    else if (type_ == TYPE_ULONG) {
        return TYPE_ULONG_STRING;
    }
    else if (type_ == TYPE_LONGLONG) {
        return TYPE_LONGLONG_STRING;
    }
    else if (type_ == TYPE_ULONGLONG) {
        return TYPE_ULONGLONG_STRING;
    }
    else if (type_ == TYPE_DOUBLE) {
        return TYPE_DOUBLE_STRING;
    }
    return TYPE_STRING_STRING;
}

ComplexTypedValue*
TypedValue::complexType() const {
    return complex_.get();
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

const std::string&
TypedValue::simpleValue() const {
    return value_;
}

void
TypedValue::serialize(std::string &result) const {
    if (complex_.get()) {
        complex_->serialize(result);
    }
    else {
        result.clear();
        boost::uint32_t var = value_.size();
        result.reserve(var + sizeof(var));
        result.append((char*)&var, sizeof(var));
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

void
TypedValue::visitAsString(TypedValueVisitor *visitor) const {
    if (NULL != complex_.get()) {
        complex_->visit(visitor);
        return;
    }
    visitor->visitString(value_);
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
    if (value.undefined()) {
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

ComplexTypedValue::~ComplexTypedValue()
{}

const std::string ArrayTypedValue::TYPE = "Array";

ArrayTypedValue::ArrayTypedValue(const std::vector<std::string> &value) : value_(value)
{}

ArrayTypedValue::ArrayTypedValue(const Range &value) {
    const char* buf = value.begin();
    const char* end = value.end();
    while (buf < end) {
        if (end - buf < (boost::int32_t)sizeof(boost::uint32_t)) {
            throw std::runtime_error("Incorrect format of array typed value");
        }
        boost::uint32_t var = *((boost::uint32_t*)buf);
        buf += sizeof(boost::uint32_t);
        if (end - buf < (boost::int64_t)var) {
            throw std::runtime_error("Incorrect format of array typed value");
        }
        value_.push_back(std::string(buf, var));
        buf += var;
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
    return value_.empty() ? StringUtils::EMPTY_STRING : value_[0];
}

void
ArrayTypedValue::serialize(std::string &result) const {
    boost::uint32_t size = 0;
    for (std::vector<std::string>::const_iterator it = value_.begin();
         it != value_.end();
         ++it) {
        size += it->size();
        size += sizeof(size);
    }
    result.clear();
    result.reserve(size);
    for (std::vector<std::string>::const_iterator it = value_.begin();
         it != value_.end();
         ++it) {
        boost::uint32_t var = it->size();
        result.append((char*)&var, sizeof(var));
        result.append(*it);
    }
}

const std::string MapTypedValue::TYPE = "Map";

MapTypedValue::MapTypedValue(const std::vector<StringUtils::NamedValue> &value) :
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
        var = *((boost::uint32_t*)buf);
        buf += sizeof(boost::uint32_t);
        if (end - buf < (boost::int64_t)var) {
            throw std::runtime_error("Incorrect format of map typed value");
        }
        Range value(buf, buf + var);
        buf += var;
        value_.push_back(StringUtils::NamedValue(
            std::string(key.begin(), key.size()),
            std::string(value.begin(), value.size())));
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
    return value_.empty() ? StringUtils::EMPTY_STRING : value_.begin()->second;
}

void
MapTypedValue::serialize(std::string &result) const {
    boost::uint32_t size = 0;
    for (std::vector<StringUtils::NamedValue>::const_iterator it = value_.begin();
         it != value_.end();
         ++it) {
        size += it->first.size();
        size += sizeof(size);
        size += it->second.size();
        size += sizeof(size);
    }
    result.clear();
    result.reserve(size);
    for (std::vector<StringUtils::NamedValue>::const_iterator it = value_.begin();
         it != value_.end();
         ++it) {
        boost::uint32_t var = it->first.size();
        result.append((char*)&var, sizeof(var));
        result.append(it->first);
        var = it->second.size();
        result.append((char*)&var, sizeof(var));
        result.append(it->second);
    }
}

} // namespace xscript
