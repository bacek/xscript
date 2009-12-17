#include "settings.h"
#include "internal/loader.h"
#include "xscript/component.h"
#include "xscript/logger_factory.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

ComponentBase::ComponentBase()
        : loader_(Loader::instance()) {
}

ComponentBase::~ComponentBase() {
}


void
ComponentBase::ResourceTraits::destroy(ComponentBase *component) {
    // Acquire loader to avoid premature unload of shared library.
    boost::shared_ptr<Loader> loader = component->loader();
    boost::checked_delete(component);
};

ComponentBase::ComponentMapType&
ComponentBase::componentMap() {
    if (components_ == NULL) {
        static ComponentMapType *map = new ComponentMapType();
        components_ = map;
    }
    return *components_;
}

ComponentBase::ComponentMapType* ComponentBase::components_ = NULL;
ComponentBase* const ComponentBase::ResourceTraits::DEFAULT_VALUE = static_cast<ComponentBase*>(NULL);

} // namespace xscript
