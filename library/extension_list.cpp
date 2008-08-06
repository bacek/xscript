#include "settings.h"

#include <sstream>
#include <stdexcept>

#include <boost/checked_delete.hpp>
#include <boost/current_function.hpp>

#include "xscript/logger.h"
#include "xscript/xslt_extension.h"

#include "details/expect.h"
#include "details/loader.h"
#include "details/extension_list.h"
#include "xscript/thread_pool.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

ExtensionList::ExtensionList() {
    loader_ = Loader::instance();
}

ExtensionList::~ExtensionList() {
    ThreadPool::instance()->stop();
    std::for_each(extensions_.begin(), extensions_.end(),
                  boost::checked_deleter<Extension>());
}

void
ExtensionList::registerExtension(ExtensionHolder ext) {
    if (XSCRIPT_UNLIKELY(NULL != extension(ext->name(), ext->nsref()))) {
        std::stringstream stream;
        stream << "duplicate extension: " << ext->name();
        throw std::invalid_argument(stream.str());
    }
    extensions_.push_back(ext.get());
    try {
        XsltElementRegisterer::registerBlockInvokation(ext->name(), ext->nsref());
        ext.release();
    }
    catch (const std::exception &e) {
        log()->crit("failed to register xslt extension: %s", e.what());
        extensions_.pop_back();
        throw;
    }
}

Extension*
ExtensionList::extension(const xmlNodePtr node) const {
    const char *name = (const char*) node->name;
    const char *ref = node->ns ? (const char*) node->ns->href : NULL;
    return extension(name, ref);
}

Extension*
ExtensionList::extension(const char *name, const char *ref) const {
    if (XSCRIPT_UNLIKELY(NULL == name)) {
        return NULL;
    }
    for (std::vector<Extension*>::const_iterator i = extensions_.begin(), end = extensions_.end(); i != end; ++i) {
        if (accepts((*i), name, ref)) {
            return (*i);
        }
    }
    return NULL;
}

bool
ExtensionList::accepts(Extension *ext, const char *name, const char *ref) const {

    const char *ename = (const char*) ext->name();
    const char *extref = (const char*) ext->nsref();

    if (strncasecmp(ename, name, strlen(ename) + 1) != 0) {
        return false;
    }
    if (NULL != ref) {
        return strncasecmp(ref, extref, strlen(extref) + 1) == 0;
    }
    return true;
}

ExtensionRegisterer::ExtensionRegisterer(ExtensionHolder helper) throw () {
    try {
        ExtensionList::instance()->registerExtension(helper);
    }
    catch (const std::exception &e) {
        log()->crit("caught exception while registering module: %s", e.what());
    }
}

} // namespace xscript
