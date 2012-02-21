#include "settings.h"
#include <cassert>
#include <time.h>

#include "xscript/typed_cache.h"

#include <boost/lexical_cast.hpp>

#include "xscript/cached_object.h"
#include "xscript/tag.h"
#include "xscript/doc_cache.h"
#include "xscript/typed_map.h"


namespace xscript {

TypedCachedObject::TypedCachedObject(const std::string &key) : key_(key) {
    assert(!key.empty());
    CachedObject::setCacheStrategyLocal();
}

TypedCachedObject::~TypedCachedObject() {
}


TypedCacheData::TypedCacheData() {
}

TypedCacheData::TypedCacheData(const std::string &key, TypedValue &value) : key_(key) {
    assert(!key.empty());
    value.swap(value_);
}

TypedCacheData::~TypedCacheData() {
}


TypedCache::TypedCache() {
}

TypedCache::~TypedCache() {
}

TypedCache*
TypedCache::instance() {
    static TypedCache cache;
    return &cache;
}

void
TypedCache::createUsageCounter(boost::shared_ptr<StatInfo> info) {
    (void)info;
    //info->usageCounter_ = TaggedCacheUsageCounterFactory::instance()->createTypedCounter("disgrace");
}
    
std::string
TypedCache::name() const {
    return "cache";
}

boost::shared_ptr<TypedCacheData>
TypedCache::load(CacheContext *cache_ctx, Tag &tag) {
    boost::shared_ptr<CacheData> cache_data(new TypedCacheData());
    bool res = loadDocImpl(NULL, cache_ctx, tag, cache_data);
    if (!res) {
        return boost::shared_ptr<TypedCacheData>();
    }
    return boost::dynamic_pointer_cast<TypedCacheData>(cache_data);
}

bool
TypedCache::save(CacheContext *cache_ctx, const Tag &tag,
    const boost::shared_ptr<TypedCacheData> &cache_data) {
    return saveDocImpl(NULL, cache_ctx, tag, boost::dynamic_pointer_cast<CacheData>(cache_data));
}

} // namespace xscript
