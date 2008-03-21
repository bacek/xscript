#include "settings.h"
#include "xscript/state_value.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

const std::string StateValue::TYPE_BOOL_STRING = "Boolean";
const std::string StateValue::TYPE_LONG_STRING = "Long";
const std::string StateValue::TYPE_ULONG_STRING = "ULong";
const std::string StateValue::TYPE_LONGLONG_STRING = "LongLong";
const std::string StateValue::TYPE_ULONGLONG_STRING = "ULongLong";
const std::string StateValue::TYPE_DOUBLE_STRING = "Double";
const std::string StateValue::TYPE_STRING_STRING = "String";

StateValue::StateValue() :
	type_(TYPE_STRING)
{
}

StateValue::StateValue(int type, const std::string &value) :
	type_(type), value_(value)
{
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

} // namespace xscript
