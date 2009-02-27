#include "settings.h"
#include "xscript/stylesheet.h"
#include "xscript/stylesheet_cache.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {


void
StylesheetCache::clear() {
}

void
StylesheetCache::erase(const std::string &name) {
    (void)name;
}

boost::shared_ptr<Stylesheet>
StylesheetCache::fetch(const std::string &name) {
    (void)name;
    return boost::shared_ptr<Stylesheet>();
}

void
StylesheetCache::store(const std::string &name, const boost::shared_ptr<Stylesheet> &stylesheet) {
    (void)name;
    (void)stylesheet;
}

boost::mutex*
StylesheetCache::getMutex(const std::string &name) {
    (void)name;
    return NULL;
}

static ComponentRegisterer<StylesheetCache> reg_;

} // namespace xscript
