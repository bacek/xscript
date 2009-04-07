#include "settings.h"

#include <boost/current_function.hpp>
#include <boost/bind.hpp>

#include "xscript/control_extension.h"
#include "xscript/doc_cache.h"
#include "xscript/doc_cache_strategy.h"
#include "xscript/logger.h"
#include "xscript/profiler.h"
#include "xscript/xml_util.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {


class DocCacheBlock : public Block {
public:
    DocCacheBlock(const ControlExtension *ext, Xml *owner, xmlNodePtr node, const DocCacheBase& doc_cache)
            : Block(ext, owner, node), doc_cache_(doc_cache) {
    }

    XmlDocHelper call(Context *, boost::any &) throw (std::exception) {
        return doc_cache_.createReport();
    }
private:
    const DocCacheBase &doc_cache_;
};


DocCacheBase::DocCacheBase() {
}

DocCacheBase::~DocCacheBase() {
}

void
DocCacheBase::init(const Config *config) {
    ControlExtension::Constructor f = boost::bind(boost::mem_fn(&DocCache::createBlock), this, _1, _2, _3);
    ControlExtension::registerConstructor(name().append("-stat"), f);
    
    TaggedCacheUsageCounterFactory::instance()->init(config);
    
    for(StrategyMap::iterator i = strategies_.begin(); i != strategies_.end(); ++i) {
        std::string builder_name = name() + "-" + i->first->name();
        std::auto_ptr<StatBuilder> builder(new StatBuilder(builder_name));
        boost::shared_ptr<StatInfo> stat_info(new StatInfo());
        stat_info->hitCounter_ = AverageCounterFactory::instance()->createCounter("hits");
        stat_info->missCounter_ = AverageCounterFactory::instance()->createCounter("miss");
        stat_info->saveCounter_ = AverageCounterFactory::instance()->createCounter("save");
        
        createUsageCounter(stat_info);
        
        builder->addCounter(stat_info->hitCounter_.get());
        builder->addCounter(stat_info->missCounter_.get());
        builder->addCounter(stat_info->saveCounter_.get());
        builder->addCounter(stat_info->usageCounter_.get());
        
        i->first->fillStatBuilder(builder.get());
        
        stat_info->statBuilder_ = builder;        
        i->second = stat_info;
    }
}

void
DocCacheBase::addStrategy(DocCacheStrategy* strategy, const std::string& name) {
    // FIXME Add into proper position
    (void)name;
    log()->debug("%s %s", BOOST_CURRENT_FUNCTION, name.c_str());
    strategies_.push_back(std::make_pair(strategy, boost::shared_ptr<StatInfo>()));
}

time_t
DocCacheBase::minimalCacheTime() const {
    // FIXME.Do we actually need this in public part? Better to embed logick
    // of minimalCacheTime into saveDoc.
    return 0;
}

bool
DocCacheBase::loadDoc(const Context *ctx, const Object *obj, Tag &tag, XmlDocHelper &doc) {
    // FIXME Add saving of loaded doc into higher-order caches.
    log()->debug("%s", BOOST_CURRENT_FUNCTION);
    bool loaded = false;
    StrategyMap::iterator i = strategies_.begin();
    while ( !loaded && i != strategies_.end()) {
        DocCacheStrategy* strategy = i->first;
        std::auto_ptr<TagKey> key = strategy->createKey(ctx, obj);
        
        boost::function<bool()> f =
            boost::bind(&DocCacheStrategy::loadDoc, strategy,
                    boost::cref(key.get()), boost::ref(tag), boost::ref(doc));
        
        std::pair<bool, uint64_t> res = profile(f);
        
        if (res.first) {
            i->second->usageCounter_->fetchedHit(ctx, obj);
            i->second->hitCounter_->add(res.second);
        }
        else {
            i->second->usageCounter_->fetchedMiss(ctx, obj);
            i->second->missCounter_->add(res.second);
        }
        
        loaded = res.first; 
        ++i;
    }

    if (loaded) {
        --i; // Do not store in cache from doc was loaded.
        for (StrategyMap::iterator j = strategies_.begin(); j != i; ++j) {
            DocCacheStrategy* strategy = j->first;
            std::auto_ptr<TagKey> key = strategy->createKey(ctx, obj);
            
            boost::function<bool()> f =
                boost::bind(&DocCacheStrategy::saveDoc, strategy,
                        boost::cref(key.get()), boost::cref(tag), boost::cref(doc));
            std::pair<bool, uint64_t> res = profile(f);
            i->second->saveCounter_->add(res.second);            
        }
    }

    return loaded;
}

bool
DocCacheBase::saveDoc(const Context *ctx, const Object *obj, const Tag& tag, const XmlDocHelper &doc) {
    bool saved = false;
    for (StrategyMap::iterator i = strategies_.begin();
         i != strategies_.end();
         ++i) {
        DocCacheStrategy* strategy = i->first;
        std::auto_ptr<TagKey> key = strategy->createKey(ctx, obj);
        
        boost::function<bool()> f =
            boost::bind(&DocCacheStrategy::saveDoc, strategy,
                    boost::cref(key.get()), boost::cref(tag), boost::cref(doc));
        
        std::pair<bool, uint64_t> res = profile(f);
        i->second->saveCounter_->add(res.second); 
        
        saved |= res.first;
    }
    return saved;
}


std::auto_ptr<Block>
DocCacheBase::createBlock(const ControlExtension *ext, Xml *owner, xmlNodePtr node) {
    return std::auto_ptr<Block>(new DocCacheBlock(ext, owner, node, *this));
}

XmlDocHelper
DocCacheBase::createReport() const {
    XmlDocHelper doc(xmlNewDoc((const xmlChar*) "1.0"));
    XmlUtils::throwUnless(NULL != doc.get());

    xmlNodePtr root = xmlNewDocNode(doc.get(), NULL, (const xmlChar*)name().c_str(), NULL);
    XmlUtils::throwUnless(NULL != root);

    xmlDocSetRootElement(doc.get(), root);

    // Sorted list of strategies
    for (StrategyMap::const_iterator s = strategies_.begin();
         s != strategies_.end();
         ++s) {
        XmlDocHelper report = s->second->statBuilder_->createReport();
        XmlNodeHelper node(xmlCopyNode(xmlDocGetRootElement(report.get()), 1));
        xmlAddChild(root, node.get());
        node.release();
    }

    return doc;
}

DocCache::DocCache() {
}

DocCache::~DocCache() {
}

void
DocCache::createUsageCounter(boost::shared_ptr<StatInfo> info) {
    info->usageCounter_ = TaggedCacheUsageCounterFactory::instance()->createBlockCounter("usage");
}

std::string
DocCache::name() const {
    return "block-cache";
}

PageCache::PageCache() {
}

PageCache::~PageCache() {
}

void
PageCache::createUsageCounter(boost::shared_ptr<StatInfo> info) {
    info->usageCounter_ = TaggedCacheUsageCounterFactory::instance()->createScriptCounter("usage");
}

std::string
PageCache::name() const {
    return "page-cache";
}

} // namespace xscript
