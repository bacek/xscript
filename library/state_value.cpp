#include "settings.h"
#include "xscript/state_value.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

StateValue::StateValue() :
	type_(TYPE_STRING)
{
}

StateValue::StateValue(int type, const std::string &value) :
	type_(type), value_(value)
{
}

} // namespace xscript
