#include "settings.h"
#include "xscript/extension.h"
#include "xscript/logger_factory.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

Extension::Extension()
        : logger_(LoggerFactory::instance()->getDefaultLogger()) {
    assert(logger_);
}

Extension::~Extension() {
}

void
Extension::ExtensionResourceTraits::destroy(Extension *ext) {
    // Acquire loader to avoid premature unload of shared library.
    boost::shared_ptr<Loader> loader = ext->loader();
    boost::checked_delete(ext);
};

Extension* const Extension::ExtensionResourceTraits::DEFAULT_VALUE = static_cast<Extension*>(NULL);

} // namespace xscript
