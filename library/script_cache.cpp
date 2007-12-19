#include "settings.h"
#include "xscript/script.h"
#include "xscript/script_cache.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

ScriptCache::ScriptCache()
{
}

ScriptCache::~ScriptCache() {
}

void
ScriptCache::init(const Config *config) {
}

void
ScriptCache::clear() {
}

void
ScriptCache::erase(const std::string &name) {
}

boost::shared_ptr<Script>
ScriptCache::fetch(const std::string &name) {
	return boost::shared_ptr<Script>();
}

void
ScriptCache::store(const std::string &name, const boost::shared_ptr<Script> &script) {
}

} // namespace xscript
