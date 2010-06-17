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

TypedValue::TypedValue() : type_(TYPE_STRING)
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

TypedValue::TypedValue(const std::map<std::string, std::string> &value) :
    type_(TYPE_MAP), complex_(new MapTypedValue(value))
{}

TypedValue::TypedValue(unsigned int type, const Range &value) :
    type_(type)
{
    if (type >= TYPE_BOOL && type <= TYPE_STRING) {
        value_.assign(value.begin(), value.size());
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

std::string
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
        result.assign(value_);
    }
}

void
TypedValue::visit(TypedValueVisitor *visitor, bool string_prefer) const {
    if (NULL != complex_.get()) {
        complex_->visit(visitor);
        return;
    }

    if (string_prefer) {
        visitor->visitString(value_);
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

std::string
TypedMap::asString(const std::string &name) const {
    return getTypedValue(name).asString();
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
    return getTypedValue(name).asBool();
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

TypedValue
TypedMap::find(const std::string &name) const {
    const TypedValue& value = getTypedValue(name);
    if (NULL == value.complex_.get()) {
        return value;
    }
    TypedValue result = value;
    result.complex_.reset(value.complex_->clone());
    return result;
}

const TypedValue&
TypedMap::getTypedValue(const std::string &name) const {
    TypedValueMap::const_iterator it = values_.find(name);
    if (values_.end() == it) {
        throw std::invalid_argument("nonexistent typed value: " + name);
    }
    return it->second;
}

std::string
TypedMap::asString(const std::string &name, const std::string &default_value) const {
    TypedValueMap::const_iterator it = values_.find(name);
    if (values_.end() != it) {
        return it->second.asString();
    }
    return default_value;
}

bool
TypedMap::find(const std::string &name, TypedValue &result) const {
    TypedValueMap::const_iterator it = values_.find(name);
    if (values_.end() == it) {
        return false;
    }
    result = it->second;
    if (it->second.complex_.get()) {
        result.complex_.reset(it->second.complex_->clone());
    }
    return true;
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
    if (!value.empty()) {
        Range tail = value;
        Range head;
        bool flag = true;
        do {
            flag = split(tail, Parser::RN_RANGE, head, tail);
            value_.push_back(std::string(head.begin(), head.size()));
        }
        while(flag);
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

std::string
ArrayTypedValue::asString() const {
    return value_.empty() ? StringUtils::EMPTY_STRING : value_[0];
}

void
ArrayTypedValue::serialize(std::string &result) const {
    result.clear();
    for (std::vector<std::string>::const_iterator it = value_.begin(), it_beg = it;
         it != value_.end();
         ++it) {
        if (it != it_beg) {
            result.append("\r\n");
        }
        result.append(*it);
    }
}

ComplexTypedValue*
ArrayTypedValue::clone() const {
    return new ArrayTypedValue(*this);
}


const std::string MapTypedValue::TYPE = "Map";

MapTypedValue::MapTypedValue(const std::map<std::string, std::string> &value) :
    value_(value)
{}

MapTypedValue::MapTypedValue(const Range &value) {
    if (!value.empty()) {
        int i = 0;
        Range tail = value;
        Range head, key;
        bool flag = true;
        do {
            flag = split(tail, Parser::RN_RANGE, head, tail);
            if (i == 0) {
                key = head;
                ++i;
            }
            else {
                value_.insert(std::make_pair(
                    std::string(key.begin(), key.size()),
                    std::string(head.begin(), head.size())));
                --i;
            }
        }
        while(flag);
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

std::string
MapTypedValue::asString() const {
    return value_.empty() ? StringUtils::EMPTY_STRING : value_.begin()->second;
}

void
MapTypedValue::serialize(std::string &result) const {
    result.clear();
    for (std::map<std::string, std::string>::const_iterator it = value_.begin(), it_beg = it;
         it != value_.end();
         ++it) {
        if (it != it_beg) {
            result.append("\r\n");
        }
        result.append(it->first);
        result.append("\r\n");
        result.append(it->second);
    }
}

ComplexTypedValue*
MapTypedValue::clone() const {
    return new MapTypedValue(*this);
}



} // namespace xscript
