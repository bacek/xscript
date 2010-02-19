#include "settings.h"
#include "xscript/script.h"
#include "xscript/script_cache.h"
#include "xscript/script_factory.h"
#include "xscript/string_utils.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

ScriptFactory::ScriptFactory() {
}

ScriptFactory::~ScriptFactory() {
}

boost::shared_ptr<Script>
ScriptFactory::create(const std::string &name) {
    return boost::shared_ptr<Script>(new Script(name));
}

boost::shared_ptr<Script>
ScriptFactory::createScript(const std::string &name) {
    ScriptCache *cache = ScriptCache::instance();
    boost::shared_ptr<Script> script = cache->fetch(name);
    if (NULL != script.get()) {
        return script;
    }
  
    boost::mutex *mutex = cache->getMutex(name);
    if (NULL == mutex) {
        return createWithParse(name);
    }
    
    boost::mutex::scoped_lock lock(*mutex);
    script = cache->fetch(name);
    if (NULL != script.get()) {
        return script;
    }

    return createWithParse(name);
}

boost::shared_ptr<Script>
ScriptFactory::createScriptFromXml(const std::string &name, const std::string &xml) {
    boost::shared_ptr<Script> script = ScriptFactory::instance()->create(name);
    script->parseFromXml(xml);
    return script;
}

boost::shared_ptr<Script>
ScriptFactory::createScriptFromXmlNode(const std::string &name, xmlNodePtr node) {
    boost::shared_ptr<Script> script = ScriptFactory::instance()->create(name);
    script->parseFromXml(node);
    return script;
}

boost::shared_ptr<Script>
ScriptFactory::createWithParse(const std::string &name) {
    boost::shared_ptr<Script> script = ScriptFactory::instance()->create(name);
    script->parse();
    ScriptCache::instance()->store(name, script);
    return script;
}

static ComponentRegisterer<ScriptFactory> reg;

} // namespace xscript
