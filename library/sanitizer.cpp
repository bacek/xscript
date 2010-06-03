#include "settings.h"
#include <stdexcept>
#include "xscript/policy.h"
#include "xscript/sanitizer.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

Sanitizer::Sanitizer() {
}

Sanitizer::~Sanitizer() {
}

std::string
Sanitizer::sanitize(const Range &range, const std::string &base_url, int line_limit) {
	(void)base_url;
	(void)line_limit;
	Policy::instance()->useDefaultSanitizer();
    return std::string(range.begin(), range.end());
}

static ComponentRegisterer<Sanitizer> reg;

} // namespace xscript
