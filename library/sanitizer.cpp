#include "settings.h"
#include <stdexcept>
#include "xscript/sanitizer.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

Sanitizer::Sanitizer() 
{
}

Sanitizer::~Sanitizer() {
}

std::string
Sanitizer::sanitize(const Range &range) {
	return std::string(range.begin(), range.end());
}

REGISTER_COMPONENT(Sanitizer);

} // namespace xscript
