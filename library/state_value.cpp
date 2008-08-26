#include "settings.h"
#include "internal/algorithm.h"
#include "xscript/range.h"
#include "xscript/state_value.h"

#include <boost/lexical_cast.hpp>

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

const std::string StateValue::TYPE_BOOL_STRING = "Boolean";
const std::string StateValue::TYPE_LONG_STRING = "Long";
const std::string StateValue::TYPE_ULONG_STRING = "ULong";
const std::string StateValue::TYPE_LONGLONG_STRING = "LongLong";
const std::string StateValue::TYPE_ULONGLONG_STRING = "ULongLong";
const std::string StateValue::TYPE_DOUBLE_STRING = "Double";
const std::string StateValue::TYPE_STRING_STRING = "String";

StateValue::StateValue() :
        type_(TYPE_STRING) {
}

StateValue::StateValue(int type, const std::string &value) :
        type_(type), value_(value) {
}

const std::string&
StateValue::stringType() const {

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
StateValue::asBool() const {

    if (trim(createRange(value_)).empty()) {
        return false;
    }
    else if (type_ == StateValue::TYPE_STRING) {
        return true;
    }
    else if (type_ == StateValue::TYPE_DOUBLE) {
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

} // namespace xscript
