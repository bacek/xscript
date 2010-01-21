#include "settings.h"

#include <boost/bind.hpp>
#include <boost/tokenizer.hpp>

#include "xscript/config.h"
#include "xscript/control_extension.h"
#include "xscript/doc_cache.h"
#include "xscript/doc_cache_strategy.h"
#include "xscript/logger.h"
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
    CachedObject::addDefaultStrategy(strategy->strategy());
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
    
    std::vector<std::string> v;
    config->subKeys("/xscript/page-cache-strategies/strategy", v);
    for(std::vector<std::string>::iterator it = v.begin(), end = v.end(); it != end; ++it) {
        std::string name;
        try {
            name = config->as<std::string>(*it + "/@id");
        }
        catch(const std::runtime_error &e) {
            log()->error("Cannot find strategy id: %s", e.what());
            continue;
        }
        boost::shared_ptr<CacheStrategy> cache_strategy(new CacheStrategy());
        
        for(HandlerMapType::iterator handler_it = page_strategy_handlers_.begin();
            handler_it != page_strategy_handlers_.end();
            ++handler_it) {
            std::auto_ptr<SubCacheStrategy> sub_strategy =
                handler_it->second->create(config, *it + "/" + handler_it->first);
            
            if (NULL != sub_strategy.get()) {
                cache_strategy->add(sub_strategy);
            }
        }
        page_cache_strategies_.insert(std::make_pair(name, cache_strategy));
    }
    
    v.clear();
    config->subKeys("/xscript/block-cache-strategies/strategy", v);
    for (std::vector<std::string>::iterator it = v.begin(), end = v.end(); it != end; ++it) {
        
        std::string name = config->as<std::string>(*it + "/@id");
        boost::shared_ptr<CacheStrategy> cache_strategy(new CacheStrategy());
        
        for(HandlerMapType::iterator handler_it = block_strategy_handlers_.begin();
            handler_it != block_strategy_handlers_.end();
            ++handler_it) {
            
            std::auto_ptr<SubCacheStrategy> sub_strategy =
                handler_it->second->create(config, *it + "/" + handler_it->first);
            
            if (NULL != sub_strategy.get()) {
                cache_strategy->add(sub_strategy);
            }
        }
        
        block_cache_strategies_.insert(std::make_pair(name, cache_strategy));
    }
}

void
CacheStrategyCollector::addBlockStrategyHandler(const std::string &tag,
        const boost::shared_ptr<SubCacheStrategyFactory> &handler) {
    if (block_strategy_handlers_.end() != block_strategy_handlers_.find(tag)) {
        throw std::runtime_error("Block strategy handler added already for tag: " + tag);
    }
    block_strategy_handlers_.insert(std::make_pair(tag, handler));
}

void
CacheStrategyCollector::addPageStrategyHandler(const std::string &tag,
        const boost::shared_ptr<SubCacheStrategyFactory> &handler) {
    if (page_strategy_handlers_.end() != page_strategy_handlers_.find(tag)) {
        throw std::runtime_error("Page strategy handler added already for tag: " + tag);
    }
    page_strategy_handlers_.insert(std::make_pair(tag, handler));
}

boost::shared_ptr<CacheStrategy>
CacheStrategyCollector::blockStrategy(const std::string &name) const {
    StrategyMapType::const_iterator it = block_cache_strategies_.find(name);
    if (block_cache_strategies_.end() == it) {
        return boost::shared_ptr<CacheStrategy>();
    }
    return it->second;
}

boost::shared_ptr<CacheStrategy>
CacheStrategyCollector::pageStrategy(const std::string &name) const {
    StrategyMapType::const_iterator it = page_cache_strategies_.find(name);
    if (page_cache_strategies_.end() == it) {
        return boost::shared_ptr<CacheStrategy>();
    }
    return it->second;
}

} // namespace xscript
