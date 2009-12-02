#include "settings.h"

#include <boost/bind.hpp>
#include <boost/tokenizer.hpp>

#include "xscript/control_extension.h"
#include "xscript/doc_cache.h"
#include "xscript/doc_cache_strategy.h"
#include "xscript/tag.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

TagKey::TagKey() {
}

TagKey::~TagKey() {
}

DocCacheStrategy::DocCacheStrategy() {
}

DocCacheStrategy::~DocCacheStrategy() {
}

void
DocCacheStrategy::init(const Config *config) {
    (void)config;
}

bool
DocCacheStrategy::loadDoc(const TagKey *key, Tag &tag, XmlDocSharedHelper &doc, bool need_copy) {
    return loadDocImpl(key, tag, doc, need_copy);
}

bool
DocCacheStrategy::saveDoc(const TagKey *key, const Tag& tag, const XmlDocSharedHelper &doc, bool need_copy) {
    return saveDocImpl(key, tag, doc, need_copy);
}

bool
DocCacheStrategy::distributed() const {
    return false;
}

void
DocCacheStrategy::insert2Cache(const std::string &no_cache) {

    typedef boost::char_separator<char> Separator;
    typedef boost::tokenizer<Separator> Tokenizer;

    bool cache_block = true, cache_page = true;
    Tokenizer tok(no_cache, Separator(" ,"));
    for (Tokenizer::iterator it = tok.begin(), end = tok.end(); it != end; ++it) {
        if (strcasecmp(no_cache.c_str(), "page") == 0) {
            cache_page = false;
        }
        else if (strcasecmp(no_cache.c_str(), "block") == 0) {
            cache_block = false;
        }
    }
        
    if (cache_page) {
        PageCache::instance()->addStrategy(this, name());
    }
    if (cache_block) {
        DocCache::instance()->addStrategy(this, name());
    }
}

void
DocCacheStrategy::fillStatBuilder(StatBuilder *builder) {
    (void)builder;
}

CacheStrategyCollector::CacheStrategyCollector() {
}

CacheStrategyCollector::~CacheStrategyCollector() {
}

CacheStrategyCollector*
CacheStrategyCollector::instance() {
    static CacheStrategyCollector collector;
    return &collector;
}

void
CacheStrategyCollector::addStrategy(DocCacheStrategy* strategy, const std::string& name) {
    strategies_.push_back(std::make_pair(strategy, name));
}

void
CacheStrategyCollector::init(const Config *config) {
    for(std::vector<std::pair<DocCacheStrategy*, std::string> >::iterator it = strategies_.begin();
        it != strategies_.end();
        ++it) {
        it->first->init(config);
    }
    DocCache::instance()->init(config);
    PageCache::instance()->init(config);
}

} // namespace xscript
