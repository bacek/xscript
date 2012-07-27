#include "settings.h"

#include "xscript/extension.h"
#include "xscript/logger_factory.h"
#include <internal/extension_list.h>

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

bool
Extension::allowEmptyNamespace() const {
    return true;
}

Extension* const Extension::ExtensionResourceTraits::DEFAULT_VALUE = static_cast<Extension*>(NULL);

Extension*
getExtension(const char *name, const char *ref, bool allow_empty_namespace) {
    return ExtensionList::instance()->extension(name, ref, allow_empty_namespace);
}

} // namespace xscript
