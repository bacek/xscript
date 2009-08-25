#include "settings.h"

#include <boost/current_function.hpp>

#include "xscript/logger.h"
#include "xscript/stylesheet.h"
#include "xscript/stylesheet_cache.h"
#include "xscript/stylesheet_factory.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

StylesheetFactory::StylesheetFactory() {
}

StylesheetFactory::~StylesheetFactory() {
}

boost::shared_ptr<Stylesheet>
StylesheetFactory::create(const std::string &name) {
    return boost::shared_ptr<Stylesheet>(new Stylesheet(name));
}

boost::shared_ptr<Stylesheet>
StylesheetFactory::createStylesheet(const std::string &name) {

    log()->debug("%s creating stylesheet %s", BOOST_CURRENT_FUNCTION, name.c_str());

    StylesheetCache *cache = StylesheetCache::instance();
    boost::shared_ptr<Stylesheet> stylesheet = cache->fetch(name);
    if (NULL != stylesheet.get()) {
        return stylesheet;
    }

    boost::mutex *mutex = cache->getMutex(name);
    if (NULL == mutex) {
        return createWithParse(name);
    }

    boost::mutex::scoped_lock lock(*mutex);
    stylesheet = cache->fetch(name);
    if (NULL != stylesheet.get()) {
        return stylesheet;
    }

    return createWithParse(name);
}

boost::shared_ptr<Stylesheet>
StylesheetFactory::createWithParse(const std::string &name) {
    boost::shared_ptr<Stylesheet> stylesheet = StylesheetFactory::instance()->create(name);
    stylesheet->parse();
    StylesheetCache::instance()->store(name, stylesheet);
    return stylesheet;
}

static ComponentRegisterer<StylesheetFactory> reg;

} // namespace xscript
