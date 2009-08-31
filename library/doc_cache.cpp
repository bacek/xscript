#include "settings.h"

#include <boost/bind.hpp>
#include <boost/current_function.hpp>
#include <boost/lexical_cast.hpp>

#include "xscript/average_counter.h"
#include "xscript/doc_cache.h"
#include "xscript/doc_cache_strategy.h"
#include "xscript/logger.h"
#include "xscript/profiler.h"
#include "xscript/stat_builder.h"
#include "xscript/tagged_cache_usage_counter.h"
#include "xscript/xml_util.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class DocCacheBase::StatInfo {
public:
    std::auto_ptr<StatBuilder> statBuilder_;
    std::auto_ptr<AverageCounter> hitCounter_;
    std::auto_ptr<AverageCounter> missCounter_;
    std::auto_ptr<AverageCounter> saveCounter_;
    std::auto_ptr<TaggedCacheUsageCounter> usageCounter_;
};

class DocCacheBase::DocCacheData {
public:
    DocCacheData(DocCacheBase *owner);
    ~DocCacheData();

    std::auto_ptr<Block> createBlock(const ControlExtension *ext, Xml *owner, xmlNodePtr node);
    XmlDocHelper createReport() const;

    class DocCacheBlock : public Block {
    public:
        DocCacheBlock(const ControlExtension *ext, Xml *owner, xmlNodePtr node, const DocCacheData &cache_data)
                : Block(ext, owner, node), cache_data_(cache_data) {
        }

        XmlDocHelper call(boost::shared_ptr<Context>, boost::any &) throw (std::exception) {
            return cache_data_.createReport();
        }
    private:
        const DocCacheData &cache_data_;
    };
    
    DocCacheBase *owner_;
    std::vector<std::string> strategiesOrder_;
    typedef std::vector<std::pair<DocCacheStrategy*, boost::shared_ptr<StatInfo> > > StrategyMap;
    StrategyMap strategies_;
};

DocCacheBase::DocCacheData::DocCacheData(DocCacheBase *owner) : owner_(owner)
{}

DocCacheBase::DocCacheData::~DocCacheData()
{}

std::auto_ptr<Block>
DocCacheBase::DocCacheData::createBlock(const ControlExtension *ext, Xml *owner, xmlNodePtr node) {
    return std::auto_ptr<Block>(new DocCacheBlock(ext, owner, node, *this));
}

XmlDocHelper
DocCacheBase::DocCacheData::createReport() const {
    XmlDocHelper doc(xmlNewDoc((const xmlChar*) "1.0"));
    XmlUtils::throwUnless(NULL != doc.get());

    xmlNodePtr root = xmlNewDocNode(doc.get(), NULL, (const xmlChar*)owner_->name().c_str(), NULL);
    XmlUtils::throwUnless(NULL != root);

    xmlDocSetRootElement(doc.get(), root);

    // Sorted list of strategies
    for(DocCacheData::StrategyMap::const_iterator s = strategies_.begin();
        s != strategies_.end();
        ++s) {
        XmlDocHelper report = s->second->statBuilder_->createReport();
        XmlNodeHelper node(xmlCopyNode(xmlDocGetRootElement(report.get()), 1));
        boost::uint64_t hits = s->second->hitCounter_->count();
        boost::uint64_t misses = s->second->missCounter_->count();
        if (hits > 0 || misses > 0) {
            double hit_ratio = (double)hits/((double)hits + (double)misses);
            xmlNewProp(node.get(), (const xmlChar*)"hit-ratio",
                (const xmlChar*)boost::lexical_cast<std::string>(hit_ratio).c_str());
        }
        xmlAddChild(root, node.get());
        node.release();
    }

    return doc;
}

DocCacheBase::DocCacheBase() : data_(new DocCacheData(this)) {
}

DocCacheBase::~DocCacheBase() {
    delete data_;
}

void
DocCacheBase::init(const Config *config) {
    ControlExtension::Constructor f = boost::bind(boost::mem_fn(&DocCacheData::createBlock), data_, _1, _2, _3);
    ControlExtension::registerConstructor(name().append("-stat"), f);
    
    TaggedCacheUsageCounterFactory::instance()->init(config);
    
    for(DocCacheData::StrategyMap::iterator i = data_->strategies_.begin();
        i != data_->strategies_.end();
        ++i) {
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
DocCacheBase::addStrategy(DocCacheStrategy *strategy, const std::string &name) {
    // FIXME Add into proper position
    (void)name;
    log()->debug("%s %s", BOOST_CURRENT_FUNCTION, name.c_str());
    data_->strategies_.push_back(std::make_pair(strategy, boost::shared_ptr<StatInfo>()));
}

time_t
DocCacheBase::minimalCacheTime() const {
    // FIXME.Do we actually need this in public part? Better to embed logic
    // of minimalCacheTime into saveDoc.
    return 5;
}

bool
DocCacheBase::loadDoc(const Context *ctx, const Object *obj, Tag &tag, XmlDocHelper &doc) {
    // FIXME Add saving of loaded doc into higher-order caches.
    log()->debug("%s", BOOST_CURRENT_FUNCTION);
    bool loaded = false;
    DocCacheData::StrategyMap::iterator i = data_->strategies_.begin();
    while ( !loaded && i != data_->strategies_.end()) {
        DocCacheStrategy* strategy = i->first;
        std::auto_ptr<TagKey> key = strategy->createKey(ctx, obj);
        
        boost::function<bool()> f =
            boost::bind(&DocCacheStrategy::loadDoc, strategy,
                    boost::cref(key.get()), boost::ref(tag), boost::ref(doc));
        
        std::pair<bool, uint64_t> res = profile(f);
        
        if (res.first) {
            i->second->usageCounter_->fetchedHit(ctx, obj, key.get());
            i->second->hitCounter_->add(res.second);
        }
        else {
            i->second->usageCounter_->fetchedMiss(ctx, obj, key.get());
            i->second->missCounter_->add(res.second);
        }
        
        loaded = res.first; 
        ++i;
    }

    if (loaded) {
        --i; // Do not store in cache from doc was loaded.
        for(DocCacheData::StrategyMap::iterator j = data_->strategies_.begin(); j != i; ++j) {
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
    for (DocCacheData::StrategyMap::iterator i = data_->strategies_.begin();
         i != data_->strategies_.end();
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

DocCache::DocCache() {
}

DocCache::~DocCache() {
}

DocCache*
DocCache::instance() {
    static DocCache cache;
    return &cache;
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

PageCache*
PageCache::instance() {
    static PageCache cache;
    return &cache;
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
