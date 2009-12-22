#include "settings.h"

#include <boost/bind.hpp>
#include <boost/current_function.hpp>
#include <boost/lexical_cast.hpp>

#include "xscript/average_counter.h"
#include "xscript/context.h"
#include "xscript/doc_cache.h"
#include "xscript/doc_cache_strategy.h"
#include "xscript/logger.h"
#include "xscript/profiler.h"
#include "xscript/stat_builder.h"
#include "xscript/tag.h"
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

        XmlDocHelper call(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) throw (std::exception) {
            (void)invoke_ctx;
            ControlExtension::setControlFlag(ctx.get());
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
DocCacheBase::checkTag(const Context *ctx, const Tag &tag, const char *operation) {
    if (tag.valid()) {
        return true;
    }
    
    if (ctx) {
        log()->warn("Tag is not valid while %s. Url: %s", operation,
            ctx->request()->getOriginalUrl().c_str());
    }
    else {
        log()->warn("Tag is not valid while %s.", operation);
    }
    
    return false;
}

bool
DocCacheBase::loadDoc(const Context *ctx, const CacheContext *cache_ctx, Tag &tag, XmlDocSharedHelper &doc) {
    bool res = loadDocImpl(ctx, cache_ctx, tag, doc, false);
    if (!res) {
        checkTag(ctx, tag, "loading doc from tagged cache");
    }
    return res;
}

bool
DocCacheBase::saveDoc(const Context *ctx, const CacheContext *cache_ctx, const Tag &tag, const XmlDocSharedHelper &doc) {
    if (NULL == doc->get() || NULL == xmlDocGetRootElement(doc->get())) {
        log()->warn("cannot save empty document or document with no root to tagged cache. Url: %s",
            ctx->request()->getOriginalUrl().c_str());
        return false;
    }
    
    if (!checkTag(ctx, tag, "saving doc from tagged cache")) {
        return false;
    }
    
    return saveDocImpl(ctx, cache_ctx, tag, doc, false);
}

bool
DocCacheBase::allow(const DocCacheStrategy* strategy, const CacheContext *cache_ctx) const {
    CachedObject::Strategy cache_strategy = strategy->strategy();
    if (CachedObject::UNKNOWN == cache_strategy) {
        return false;
    }
    
    //TODO: move to checkStrategy;
    if (CachedObject::DISTRIBUTED == cache_strategy) {
        return cache_ctx->allowDistributed();
    }
    
    return cache_ctx->object()->checkStrategy(cache_strategy);
}

bool
DocCacheBase::loadDocImpl(const Context *ctx, const CacheContext *cache_ctx, Tag &tag, XmlDocSharedHelper &doc, bool need_copy) {
    // FIXME Add saving of loaded doc into higher-order caches.
    log()->debug("%s", BOOST_CURRENT_FUNCTION);
    bool loaded = false;
    DocCacheData::StrategyMap::iterator i = data_->strategies_.begin();
    while ( !loaded && i != data_->strategies_.end()) {
        DocCacheStrategy* strategy = i->first;
        if (!allow(strategy, cache_ctx)) {
            ++i;
            continue;
        }
        
        std::auto_ptr<TagKey> key = strategy->createKey(ctx, cache_ctx->object());
        
        boost::function<bool()> f =
            boost::bind(&DocCacheStrategy::loadDoc, strategy,
                    boost::cref(key.get()), boost::ref(tag), boost::ref(doc), need_copy);
        
        std::pair<bool, uint64_t> res = profile(f);
        
        if (res.first) {
            i->second->usageCounter_->fetchedHit(ctx, cache_ctx->object());
            i->second->hitCounter_->add(res.second);
        }
        else {
            i->second->usageCounter_->fetchedMiss(ctx, cache_ctx->object());
            i->second->missCounter_->add(res.second);
        }
        
        loaded = res.first; 
        ++i;
    }

    if (loaded) {
        --i; // Do not store in cache from doc was loaded.
        for(DocCacheData::StrategyMap::iterator j = data_->strategies_.begin(); j != i; ++j) {
            DocCacheStrategy* strategy = j->first;
            if (!allow(strategy, cache_ctx)) {
                continue;
            }
            
            std::auto_ptr<TagKey> key = strategy->createKey(ctx, cache_ctx->object());
            
            boost::function<bool()> f =
                boost::bind(&DocCacheStrategy::saveDoc, strategy,
                        boost::cref(key.get()), boost::cref(tag), boost::cref(doc), need_copy);
            std::pair<bool, uint64_t> res = profile(f);
            i->second->saveCounter_->add(res.second);            
        }
    }

    return loaded;
}

bool
DocCacheBase::saveDocImpl(const Context *ctx, const CacheContext *cache_ctx, const Tag& tag, const XmlDocSharedHelper &doc, bool need_copy) {
    bool saved = false;
    for (DocCacheData::StrategyMap::iterator i = data_->strategies_.begin();
         i != data_->strategies_.end();
         ++i) {
        DocCacheStrategy* strategy = i->first;
        if (!allow(strategy, cache_ctx)) {
            continue;
        }
        std::auto_ptr<TagKey> key = strategy->createKey(ctx, cache_ctx->object());
        
        boost::function<bool()> f =
            boost::bind(&DocCacheStrategy::saveDoc, strategy,
                    boost::cref(key.get()), boost::cref(tag), boost::cref(doc), need_copy);
        
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
    info->usageCounter_ = TaggedCacheUsageCounterFactory::instance()->createBlockCounter("disgrace");
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
    info->usageCounter_ = TaggedCacheUsageCounterFactory::instance()->createScriptCounter("disgrace");
}

std::string
PageCache::name() const {
    return "page-cache";
}

bool
PageCache::loadDoc(const Context *ctx, const CacheContext *cache_ctx, Tag &tag, XmlDocSharedHelper &doc) {
    return loadDocImpl(ctx, cache_ctx, tag, doc, true);
}

bool
PageCache::saveDoc(const Context *ctx, const CacheContext *cache_ctx, const Tag &tag, const XmlDocSharedHelper &doc) {
    return saveDocImpl(ctx, cache_ctx, tag, doc, true);
}

CacheContext::CacheContext(const CachedObject *obj) :
    obj_(obj), allow_distributed_(true)
{}

CacheContext::CacheContext(const CachedObject *obj, bool allow_distributed) :
    obj_(obj), allow_distributed_(allow_distributed)
{}

const CachedObject*
CacheContext::object() const {
    return obj_;
}

bool
CacheContext::allowDistributed() const {
    return allow_distributed_;
}

void
CacheContext::allowDistributed(bool flag) {
    allow_distributed_ = flag;
}

} // namespace xscript
