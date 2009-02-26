#include "settings.h"

#include <sstream>
#include <stdexcept>
#include <algorithm>

#include <boost/bind.hpp>
#include <boost/checked_delete.hpp>
#include <boost/current_function.hpp>

#include "xscript/logger.h"
#include "xscript/xslt_extension.h"

#include "internal/expect.h"
#include "internal/loader.h"
#include "internal/extension_list.h"
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
ExtensionList::initContext(Context *ctx) {
    std::for_each(extensions_.begin(), extensions_.end(),
                  boost::bind(&Extension::initContext, _1, ctx));
}

void
ExtensionList::stopContext(Context *ctx) {
    std::for_each(extensions_.begin(), extensions_.end(),
                  boost::bind(&Extension::stopContext, _1, ctx));
}

void
ExtensionList::destroyContext(Context *ctx) {
    std::for_each(extensions_.begin(), extensions_.end(),
                  boost::bind(&Extension::destroyContext, _1, ctx));
}

void
ExtensionList::init(const Config *config) {
    std::for_each(extensions_.begin(), extensions_.end(),
                  boost::bind(&Extension::init, _1, config));
}

void
ExtensionList::registerExtension(ExtensionHolder ext) {
    if (XSCRIPT_UNLIKELY(NULL != extension(ext->name(), ext->nsref(), false))) {
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
ExtensionList::extension(const xmlNodePtr node, bool allow_empty_namespace) const {
    const char *name = (const char*) node->name;
    const char *ref = node->ns ? (const char*) node->ns->href : NULL;
    return extension(name, ref, allow_empty_namespace);
}

Extension*
ExtensionList::extension(const char *name, const char *ref, bool allow_empty_namespace) const {
    if (XSCRIPT_UNLIKELY(NULL == name)) {
        return NULL;
    }
    for (std::vector<Extension*>::const_iterator i = extensions_.begin(), end = extensions_.end(); i != end; ++i) {
        if (accepts((*i), name, ref, allow_empty_namespace)) {
            return (*i);
        }
    }
    return NULL;
}

bool
ExtensionList::accepts(Extension *ext, const char *name, const char *ref, bool allow_empty_namespace) const {

    const char *ename = (const char*) ext->name();
    const char *extref = (const char*) ext->nsref();

    if (strncasecmp(ename, name, strlen(ename) + 1) != 0) {
        return false;
    }
    
    if (NULL == ref) {
	if (!allow_empty_namespace) {
	    return false;
	} 
    }
    else {
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
