#include <boost/current_function.hpp>
#include "xscript/logger.h"
#include "xscript/doc_cache.h"
#include "xscript/doc_cache_strategy.h"

namespace xscript
{

DocCache::DocCache() {
}

DocCache::~DocCache() {
}

void DocCache::init(const Config *config) {
    // FIXME Read order of strategies
    config_ = config;
}

void DocCache::addStrategy(DocCacheStrategy* strategy, const std::string& name) {
    // FIXME Add into proper position
    (void)name;
    log()->debug("%s %s", BOOST_CURRENT_FUNCTION, name.c_str());
    strategies_.push_back(strategy);
    strategy->init(config_);
}

time_t DocCache::minimalCacheTime() const {
    // FIXME.Do we actually need this in public part? Better to embed logick
    // of minimalCacheTime into saveDoc.
	return 0;
}

bool DocCache::loadDoc(const Context *ctx, const TaggedBlock *block, Tag &tag, XmlDocHelper &doc) {
    // FIXME Add saving of loaded doc into higher-order caches.
    log()->debug("%s", BOOST_CURRENT_FUNCTION);
    bool loaded = false;
    for(std::vector<DocCacheStrategy*>::iterator i = strategies_.begin();
        !loaded && i != strategies_.end();
        ++i) {
        std::auto_ptr<TagKey> key = (*i)->createKey(ctx, block);
        loaded = (*i)->loadDoc(key.get(), tag, doc);
    }
	return loaded;
}

bool DocCache::saveDoc(const Context *ctx, const TaggedBlock *block, const Tag& tag, const XmlDocHelper &doc) {
    // FIXME Save doc in first cache only. Use separate thread to save in other caches.
    bool saved = false;
    for(std::vector<DocCacheStrategy*>::iterator i = strategies_.begin();
        i != strategies_.end();
        ++i) {
        std::auto_ptr<TagKey> key = (*i)->createKey(ctx, block);
        saved |= (*i)->saveDoc(key.get(), tag, doc);
    }
	return saved;
}

REGISTER_COMPONENT(DocCache);

}
