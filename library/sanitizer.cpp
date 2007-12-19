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

void
Sanitizer::init(const Config *config) {
}

std::string
Sanitizer::sanitize(const Range &range) {
	throw std::runtime_error("not implemented");
}

} // namespace xscript
