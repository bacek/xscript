#include "settings.h"

#include <boost/bind.hpp>

#include "xscript/block.h"
#include "xscript/cached_object.h"
#include "xscript/cache_strategy_collector.h"
#include "xscript/config.h"
#include "xscript/control_extension.h"
#include "xscript/doc_cache_strategy.h"
#include "xscript/logger.h"
#include "xscript/vhost_data.h"
#include "xscript/xml_util.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class CacheStrategyCollector::CacheStrategyBlock : public Block {
public:
    CacheStrategyBlock(const ControlExtension *ext, Xml *owner, xmlNodePtr node,
        CacheStrategyCollector *collector)
            : Block(ext, owner, node), collector_(collector) {
    }

    void call(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) const
        throw (std::exception) {
        ControlExtension::setControlFlag(ctx.get());

        const Config* conf = VirtualHostData::instance()->getConfig();
        std::auto_ptr<Config> config = Config::create(conf->fileName().c_str());

        CacheStrategyCollector::StrategyMapType strategies, invalid_strategies, new_strategies;
        collector_->parsePageCacheStrategies(config.get(), new_strategies);

        static boost::mutex mutex;
        {
            bool need_swap = false;
            boost::mutex::scoped_lock lock(mutex);
            typedef CacheStrategyCollector::StrategyMapType::iterator StrategyIterator;
            for (StrategyIterator it = collector_->page_strategies_.begin();
                it != collector_->page_strategies_.end();
                ++it) {
                StrategyIterator it_new = new_strategies.find(it->first);
                if (new_strategies.end() == it_new) {
                    invalid_strategies.insert(*it);
                    need_swap = true;
                }
                else if (it->second->key() != it_new->second->key()) {
                    invalid_strategies.insert(*it);
                    strategies.insert(*it_new);
                    need_swap = true;
                }
                else {
                    strategies.insert(*it);
                }
            }

            for (StrategyIterator it = new_strategies.begin();
                it != new_strategies.end();
                ++it) {
                StrategyIterator it_new = strategies.find(it->first);
                if (strategies.end() == it_new) {
                    strategies.insert(*it);
                    need_swap = true;
                }
            }

            if (need_swap) {
                collector_->swapPageStrategies(strategies);
                for(StrategyIterator it = invalid_strategies.begin();
                    it != invalid_strategies.end();
                    ++it) {
                    it->second->valid(false);
                }
            }
        }

        XmlDocHelper doc(xmlNewDoc((const xmlChar*) "1.0"));
        XmlUtils::throwUnless(NULL != doc.get());

        xmlNodePtr node = xmlNewDocNode(doc.get(), NULL,
                (const xmlChar*) "update-page-cache-strategy", (const xmlChar*) "updated");
        XmlUtils::throwUnless(NULL != node);

        xmlDocSetRootElement(doc.get(), node);
        invoke_ctx->resultDoc(doc);
    }

    static std::auto_ptr<Block> createBlock(const ControlExtension *ext, Xml *owner,
            xmlNodePtr node, CacheStrategyCollector* collector) {
        return std::auto_ptr<Block>(new CacheStrategyBlock(ext, owner, node, collector));
    }

private:
    CacheStrategyCollector* collector_;
};

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
CacheStrategyCollector::parsePageCacheStrategies(const Config *config,
        StrategyMapType &strategies) {
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
        strategies.insert(std::make_pair(name, cache_strategy));
    }
}

void
CacheStrategyCollector::swapPageStrategies(StrategyMapType &strategies) {
    boost::mutex::scoped_lock lock(page_strategies_mutex_);
    page_strategies_.swap(strategies);
}

void
CacheStrategyCollector::init(const Config *config) {

    ControlExtension::Constructor f = boost::bind(&CacheStrategyBlock::createBlock, _1, _2, _3, this);
    ControlExtension::registerConstructor("update-page-cache-strategy", f);

    for(std::vector<std::pair<DocCacheStrategy*, std::string> >::iterator it = strategies_.begin();
        it != strategies_.end();
        ++it) {
        it->first->init(config);
    }
    DocCache::instance()->init(config);
    PageCache::instance()->init(config);
    
    parsePageCacheStrategies(config, page_strategies_);

    std::vector<std::string> v;
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
        block_strategies_.insert(std::make_pair(name, cache_strategy));
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
    StrategyMapType::const_iterator it = block_strategies_.find(name);
    if (block_strategies_.end() == it) {
        return boost::shared_ptr<CacheStrategy>();
    }
    return it->second;
}

boost::shared_ptr<CacheStrategy>
CacheStrategyCollector::pageStrategy(const std::string &name) const {
    boost::mutex::scoped_lock lock(page_strategies_mutex_);
    StrategyMapType::const_iterator it = page_strategies_.find(name);
    if (page_strategies_.end() == it) {
        return boost::shared_ptr<CacheStrategy>();
    }
    return it->second;
}

} // namespace xscript
