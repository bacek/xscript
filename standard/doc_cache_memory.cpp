#include "settings.h"

#include <algorithm>
#include <cassert>
#include <list>
#include <map>

#include <boost/checked_delete.hpp>
#include <boost/crc.hpp>
#include <boost/current_function.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/thread/mutex.hpp>

#include "xscript/average_counter.h"
#include "xscript/cache_counter.h"
#include "xscript/config.h"
#include "xscript/doc_cache.h"
#include "xscript/doc_cache_strategy.h"
#include "xscript/logger.h"
#include "xscript/param.h"
#include "xscript/stat_builder.h"
#include "xscript/string_utils.h"
#include "xscript/tag.h"
#include "xscript/tagged_block.h"
#include "xscript/util.h"

#include "doc_pool.h"
#include "tag_key_memory.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class DocCacheMemory :
            public Component<DocCacheMemory>,
            public DocCacheStrategy {
public:
    DocCacheMemory();
    virtual ~DocCacheMemory();

    virtual void init(const Config *config);

    virtual time_t minimalCacheTime() const;
    virtual std::string name() const;

    virtual std::auto_ptr<TagKey> createKey(const Context *ctx, const CachedObject *obj) const;

    unsigned int maxSize() const;
    
    virtual void fillStatBuilder(StatBuilder *builder);
    
    virtual CachedObject::Strategy strategy() const;

protected:
    virtual bool loadDocImpl(const TagKey *key, Tag &tag, XmlDocSharedHelper &doc, bool need_copy);
    virtual bool saveDocImpl(const TagKey *key, const Tag& tag, const XmlDocSharedHelper &doc, bool need_copy);

private:
    DocPool* pool(const TagKey *key) const;

    static const int DEFAULT_POOL_COUNT;
    static const int DEFAULT_POOL_SIZE;
    static const time_t DEFAULT_CACHE_TIME;

private:
    time_t min_time_;
    unsigned int max_size_;
    std::vector<DocPool*> pools_;
};

const int DocCacheMemory::DEFAULT_POOL_COUNT = 16;
const int DocCacheMemory::DEFAULT_POOL_SIZE = 128;
const time_t DocCacheMemory::DEFAULT_CACHE_TIME = 5; // sec

DocCacheMemory::DocCacheMemory() :
        min_time_(Tag::UNDEFINED_TIME), max_size_(0) {
    CacheStrategyCollector::instance()->addStrategy(this, name());
}

DocCacheMemory::~DocCacheMemory() {
    std::for_each(pools_.begin(), pools_.end(), boost::checked_deleter<DocPool>());
}

std::auto_ptr<TagKey>
DocCacheMemory::createKey(const Context *ctx, const CachedObject *obj) const {
    return std::auto_ptr<TagKey>(new TagKeyMemory(ctx, obj));
}

void
DocCacheMemory::init(const Config *config) {
    DocCacheStrategy::init(config);

    assert(pools_.empty());

    Config::addForbiddenKey("/xscript/tagged-cache-memory/*");
    
    unsigned int pools = config->as<unsigned int>("/xscript/tagged-cache-memory/pools", DEFAULT_POOL_COUNT);
    unsigned int pool_size = config->as<unsigned int>("/xscript/tagged-cache-memory/pool-size", DEFAULT_POOL_SIZE);

    max_size_ = pool_size;
    for (unsigned int i = 0; i < pools; ++i) {
        char buf[20];
        snprintf(buf, 20, "pool%d", i);
        pools_.push_back(new DocPool(max_size_, buf));
    }
    min_time_ = config->as<time_t>("/xscript/tagged-cache-memory/min-cache-time", DEFAULT_CACHE_TIME);
    if (min_time_ <= 0) {
        min_time_ = DEFAULT_CACHE_TIME;
    }
    
    std::string no_cache =
        config->as<std::string>("/xscript/tagged-cache-memory/no-cache", StringUtils::EMPTY_STRING);

    insert2Cache(no_cache);
}

void
DocCacheMemory::fillStatBuilder(StatBuilder *builder) {
    for(std::vector<DocPool*>::iterator it = pools_.begin();
        it != pools_.end();
        ++it) {
        builder->addCounter((*it)->getCounter());
    }
}

time_t
DocCacheMemory::minimalCacheTime() const {
    return min_time_;
}

std::string
DocCacheMemory::name() const {
    return "memory";
}

CachedObject::Strategy
DocCacheMemory::strategy() const {
    return CachedObject::LOCAL;
}

bool
DocCacheMemory::loadDocImpl(const TagKey *key, Tag &tag, XmlDocSharedHelper &doc, bool need_copy) {
    log()->debug("loading doc in memory cache");
    DocPool *mpool = pool(key);
    assert(NULL != mpool);
    if (!mpool->loadDoc(*key, tag, doc)) {
        return false;
    }
    if (!tag.valid()) {
        log()->warn("tag is not valid while loading from memory cache");
        return false;
    }
    if (need_copy) {
        XmlDocSharedHelper res_doc(new XmlDocHelper(xmlCopyDoc(doc->get(), 1)));
        doc = res_doc;
    }
    return true;
}

bool
DocCacheMemory::saveDocImpl(const TagKey *key, const Tag &tag, const XmlDocSharedHelper &doc, bool need_copy) {
    log()->debug("saving doc in memory cache");
    DocPool *mpool = pool(key);
    assert(NULL != mpool);
    
    XmlDocSharedHelper res_doc = doc;
    if (need_copy) {
        res_doc = XmlDocSharedHelper(new XmlDocHelper(xmlCopyDoc(doc->get(), 1)));
    }
    
    return mpool->saveDoc(*key, tag, res_doc);
}

unsigned int
DocCacheMemory::maxSize() const {
    return max_size_;
}

DocPool*
DocCacheMemory::pool(const TagKey *key) const {
    assert(NULL != key);

    const unsigned int sz = pools_.size();
    assert(sz);

    return pools_[HashUtils::crc32(key->asString()) % sz];
}

static ComponentRegisterer<DocCacheMemory> reg_;

} // namespace xscript
