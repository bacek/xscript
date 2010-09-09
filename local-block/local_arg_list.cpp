#include "settings.h"

#include <stdexcept>

#include <xscript/context.h>
#include <xscript/exception.h>

#include "local_arg_list.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

LocalArgList::LocalArgList()
{}

LocalArgList::~LocalArgList() {
}

void
LocalArgList::addAs(const std::string &type, const TypedValue &value) {
    type.empty() ? add(value) : ArgList::addAs(type, value);
}

} // namespace xscript
