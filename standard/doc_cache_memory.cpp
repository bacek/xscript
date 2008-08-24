#include "settings.h"

#include <map>
#include <list>
#include <cassert>
#include <algorithm>

#include "xscript/tag.h"
#include "xscript/util.h"
#include "xscript/config.h"
#include "xscript/logger.h"
#include "xscript/param.h"
#include "xscript/tagged_block.h"
#include "xscript/doc_cache_strategy.h"
#include "xscript/doc_cache.h"
#include "xscript/cache_counter.h"
#include "xscript/memory_statistic.h"
#include "xscript/doc_cache.h"

#include "doc_pool.h"

#include <boost/crc.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/checked_delete.hpp>
#include <boost/current_function.hpp>



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

    unsigned int maxSize() const;

protected:
    virtual bool loadDocImpl(const std::string &tagKey, Tag &tag, XmlDocHelper &doc);
    virtual bool saveDocImpl(const std::string &tagKey, const Tag& tag, const XmlDocHelper &doc);

private:
    DocPool* pool(const std::string &tagKey) const;

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
    statBuilder_.setName("tagged-cache-memory");
    DocCache::instance()->addStrategy(this, "memory");
}

DocCacheMemory::~DocCacheMemory() {
    std::for_each(pools_.begin(), pools_.end(), boost::checked_deleter<DocPool>());
}

void
DocCacheMemory::init(const Config *config) {
    DocCacheStrategy::init(config);

    assert(pools_.empty());

    unsigned int pools = config->as<unsigned int>("/xscript/tagged-cache-memory/pools", DEFAULT_POOL_COUNT);
    unsigned int pool_size = config->as<unsigned int>("/xscript/tagged-cache-memory/pool-size", DEFAULT_POOL_SIZE);

    max_size_ = pool_size;
    for (unsigned int i = 0; i < pools; ++i) {
        char buf[20];
        snprintf(buf, 20, "pool%d", i);
        pools_.push_back(new DocPool(max_size_, buf));
        statBuilder_.addCounter((*pools_.rbegin())->getCounter());
        statBuilder_.addCounter((*pools_.rbegin())->getMemoryCounter());
    }
    min_time_ = config->as<time_t>("/xscript/tagged-cache-memory/min-cache-time", DEFAULT_CACHE_TIME);
    if (min_time_ <= 0) {
        min_time_ = DEFAULT_CACHE_TIME;
    }
}

time_t
DocCacheMemory::minimalCacheTime() const {
    return min_time_;
}

bool
DocCacheMemory::loadDocImpl(const std::string &tagKey, Tag &tag, XmlDocHelper &doc) {
    DocPool *mpool = pool(tagKey);
    assert(NULL != mpool);
    return mpool->loadDoc(tagKey, tag, doc);
}

bool
DocCacheMemory::saveDocImpl(const std::string &tagKey, const Tag &tag, const XmlDocHelper &doc) {
    DocPool *mpool = pool(tagKey);
    assert(NULL != mpool);
    return mpool->saveDoc(tagKey, tag, doc);
}

unsigned int
DocCacheMemory::maxSize() const {
    return max_size_;
}

DocPool*
DocCacheMemory::pool(const std::string &tagKey) const {
    const unsigned int sz = pools_.size();
    assert(sz);

    boost::crc_32_type result;
    result.process_bytes(tagKey.data(), tagKey.size());
    unsigned int index = result.checksum() % sz;
    return pools_[index];
}

//REGISTER_COMPONENT(DocCacheMemory);
namespace {
static DocCacheMemory cache_;
//static ComponentRegisterer<DocCacheMemory> reg_(new DocCacheMemory());
}

} // namespace xscript
