#include "settings.h"
#include "xscript/script.h"
#include "details/dummy_script_cache.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

void
DummyScriptCache::clear() {
}

void
DummyScriptCache::erase(const std::string &name) {
	(void)name;
}

boost::shared_ptr<Script>
DummyScriptCache::fetch(const std::string &name) {
	(void)name;
	return boost::shared_ptr<Script>();
}

void
DummyScriptCache::store(const std::string &name, const boost::shared_ptr<Script> &script) {
	(void)name;
	(void)script;
}

REGISTER_COMPONENT2(ScriptCache, DummyScriptCache);

} // namespace xscript
