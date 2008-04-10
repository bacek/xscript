#include "settings.h"
#include "details/loader.h"
#include "xscript/component.h"
#include "xscript/logger_factory.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

ComponentBase::ComponentBase() 
	: loader_(Loader::instance())
{
}

ComponentBase::~ComponentBase() {
}

} // namespace xscript
