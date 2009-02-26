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


} // namespace xscript
