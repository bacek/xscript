#include "settings.h"
#include "xscript/stylesheet.h"
#include "details/dummy_stylesheet_cache.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {


void
DummyStylesheetCache::clear() {
}

void
DummyStylesheetCache::erase(const std::string &name) {
    (void)name;
}

boost::shared_ptr<Stylesheet>
DummyStylesheetCache::fetch(const std::string &name) {
    (void)name;
    return boost::shared_ptr<Stylesheet>();
}

void
DummyStylesheetCache::store(const std::string &name, const boost::shared_ptr<Stylesheet> &stylesheet) {
    (void)name;
    (void)stylesheet;
}

boost::mutex*
DummyStylesheetCache::getMutex(const std::string &name) {
    (void)name;
    return NULL;
}

REGISTER_COMPONENT2(StylesheetCache, DummyStylesheetCache);

} // namespace xscript
