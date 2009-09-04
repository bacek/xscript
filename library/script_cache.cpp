#include "settings.h"
#include "xscript/script.h"
#include "xscript/script_cache.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

ScriptCache::ScriptCache() : statBuilder_("script-cache") {
    ControlExtension::Constructor f =
        boost::bind(boost::mem_fn(&StatBuilder::createBlock), &statBuilder_, _1, _2, _3);
    ControlExtension::registerConstructor("script-cache-stat", f);
}

StatBuilder&
ScriptCache::statBuilder() {
    return statBuilder_;
}

void
ScriptCache::clear() {
}

void
ScriptCache::erase(const std::string &name) {
    (void)name;
}

boost::shared_ptr<Script>
ScriptCache::fetch(const std::string &name) {
    (void)name;
    return boost::shared_ptr<Script>();
}

void
ScriptCache::store(const std::string &name, const boost::shared_ptr<Script> &script) {
    (void)name;
    (void)script;
}

boost::mutex*
ScriptCache::getMutex(const std::string &name) {
    (void)name;
    return NULL;
}

static ComponentRegisterer<ScriptCache> reg_;

} // namespace xscript
