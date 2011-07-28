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
#include "xscript/cache_strategy_collector.h"
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

    virtual std::auto_ptr<TagKey> createKey(const Context *ctx,
        const InvokeContext *invoke_ctx, const CachedObject *obj) const;

    unsigned int maxSize() const;
    
    virtual void fillStatBuilder(StatBuilder *builder);
    
    virtual CachedObject::Strategy strategy() const;

    virtual bool loadDoc(const TagKey *key, CacheContext *cache_ctx,
        Tag &tag, boost::shared_ptr<CacheData> &cache_data);
    virtual bool saveDoc(const TagKey *key, CacheContext *cache_ctx,
        const Tag &tag, const boost::shared_ptr<CacheData> &cache_data);

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

struct DocCleaner {
    DocCleaner(Context *ctx) : ctx_(ctx) {}
    void operator() (const boost::shared_ptr<CacheData> &cache_data) {
        cache_data->cleanup(ctx_);
    }
private:
    Context *ctx_;
};


DocCacheMemory::DocCacheMemory() :
        min_time_(Tag::UNDEFINED_TIME), max_size_(0) {
    CacheStrategyCollector::instance()->addStrategy(this, name());
}

DocCacheMemory::~DocCacheMemory() {
    std::for_each(pools_.begin(), pools_.end(), boost::checked_deleter<DocPool>());
}

std::auto_ptr<TagKey>
DocCacheMemory::createKey(const Context *ctx,
    const InvokeContext *invoke_ctx, const CachedObject *obj) const {
    return std::auto_ptr<TagKey>(new TagKeyMemory(ctx, invoke_ctx, obj));
}

void
DocCacheMemory::init(const Config *config) {
    DocCacheStrategy::init(config);

    assert(pools_.empty());

    config->addForbiddenKey("/xscript/tagged-cache-memory/*");
    
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
DocCacheMemory::loadDoc(const TagKey *key, CacheContext *cache_ctx,
    Tag &tag, boost::shared_ptr<CacheData> &cache_data) {

    log()->debug("loading doc in memory cache");
    DocPool *mpool = pool(key);
    assert(NULL != mpool);

    DocCleaner cleaner(cache_ctx->context());
    if (!mpool->loadDoc(key->asString(), tag, cache_data, cleaner)) {
        return false;
    }

    if (!DocCacheBase::checkTag(NULL, NULL, tag, "loading doc from memory cache")) {
        return false;
    }
    
    return true;
}

bool
DocCacheMemory::saveDoc(const TagKey *key, CacheContext *cache_ctx,
    const Tag &tag, const boost::shared_ptr<CacheData> &cache_data) {

    log()->debug("saving doc in memory cache");
    DocPool *mpool = pool(key);
    assert(NULL != mpool);
    DocCleaner cleaner(cache_ctx->context());
    return mpool->saveDoc(key->asString(), tag, cache_data, cleaner);
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
