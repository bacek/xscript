#include "settings.h"
#include "xscript/stylesheet.h"
#include "xscript/stylesheet_cache.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

StylesheetCache::StylesheetCache()
{
}

StylesheetCache::~StylesheetCache() {
}

void
StylesheetCache::init(const Config *config) {
}

void
StylesheetCache::clear() {
}

void
StylesheetCache::erase(const std::string &name) {
}

boost::shared_ptr<Stylesheet>
StylesheetCache::fetch(const std::string &name) {
	return boost::shared_ptr<Stylesheet>();
}

void
StylesheetCache::store(const std::string &name, const boost::shared_ptr<Stylesheet> &stylesheet) {
}

} // namespace xscript
