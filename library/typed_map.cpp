#include "settings.h"

#include <stdexcept>

#include "xscript/algorithm.h"
#include "xscript/range.h"
#include "xscript/typed_map.h"

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

TypedValue::TypedValue(int type, const std::string &value) :
    type_(type), value_(value)
{}

const std::string&
TypedValue::stringType() const {
    if (type_ == TYPE_BOOL) {
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

bool
TypedValue::asBool() const {

    if (trim(createRange(value_)).empty()) {
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
    return find(name).value();
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

void
TypedMap::values(std::map<std::string, TypedValue> &v) const {
    std::map<std::string, TypedValue> local;
    std::copy(values_.begin(), values_.end(), std::inserter(local, local.begin()));
    local.swap(v);
}

const TypedValue&
TypedMap::find(const std::string &name) const {
    TypedValueMap::const_iterator i = values_.find(name);
    if (values_.end() == i) {
        throw std::invalid_argument("nonexistent typed value: " + name);
    }
    return i->second;
}

const std::string&
TypedMap::asString(const std::string &name, const std::string &default_value) const {
    TypedValueMap::const_iterator i = values_.find(name);
    if (values_.end() != i) {
        return i->second.value();
    }
    return default_value;
}

bool
TypedMap::find(const std::string &name, TypedValue &result) const {
    TypedValueMap::const_iterator i = values_.find(name);
    if (values_.end() == i) {
        return false;
    }
    result = i->second;
    return true;
}

bool
TypedMap::is(const std::string &name) const {
    TypedValueMap::const_iterator i = values_.find(name);
    if (values_.end() != i) {
        return i->second.asBool();
    }
    return false;
}

void
TypedMap::set(const std::string &name, const TypedValue &value) {
    values_[name] = value;
}

} // namespace xscript
