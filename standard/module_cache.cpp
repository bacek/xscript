#include "settings.h"
#include <cassert>
#include <time.h>

#include "xscript/typed_cache.h"

#include <boost/lexical_cast.hpp>

#include "xscript/tag.h"
//#include "xscript/tagged_cache_usage_counter.h"
#include "xscript/external.h"
#include "xscript/typed_map.h"


namespace xscript { namespace module_cache {

static const std::string STR_KEY = "key";
static const std::string STR_VALUE = "value";
static const std::string STR_EXPIRED = "expired";


static bool
save(const ExternalFunctionContext &fctx, TypedValue &result) {

    assert(fctx.argc == 3);

    const std::string &key = fctx.argv[0].asString();
    if (key.empty()) {
        return false;
    }

    time_t t = boost::lexical_cast<time_t>(fctx.argv[2].asString());

    TypedCache *cache = TypedCache::instance();
    bool res = false;
    if (t >= cache->minimalCacheTime()) {
        TypedCachedObject obj(key);
        //obj.cacheTime(t);
        CacheContext cache_ctx(&obj, fctx.ctx, false);
        Tag tag;
        tag.expire_time = ::time(NULL) + t;
        boost::shared_ptr<TypedCacheData> cache_data(new TypedCacheData(key, fctx.argv[1]));
        res = cache->save(&cache_ctx, tag, cache_data);
    }
    TypedValue(res).swap(result);
    return true;
}

static bool
load(const ExternalFunctionContext &fctx, TypedValue &result) {

    assert(fctx.argc == 1);

    const std::string &key = fctx.argv[0].asString();
    if (key.empty()) {
        return false;
    }

    TypedCache *cache = TypedCache::instance();
    TypedCachedObject obj(key);
    CacheContext cache_ctx(&obj, fctx.ctx, false);

    Tag tag(true, Tag::EXPIRED_TIME, Tag::EXPIRED_TIME); // fake expired Tag
    boost::shared_ptr<TypedCacheData> cache_data = cache->load(&cache_ctx, tag);
    if (!cache_data.get() || tag.expired()) {
        return false;
    }

    TypedValue(cache_data->value()).swap(result);
    return true;
}

static bool
loadData(const ExternalFunctionContext &fctx, TypedValue &result) {

    assert(fctx.argc == 1);

    const std::string &key = fctx.argv[0].asString();
    if (key.empty()) {
        return false;
    }

    TypedCache *cache = TypedCache::instance();
    TypedCachedObject obj(key);
    CacheContext cache_ctx(&obj, fctx.ctx, false);
    Tag tag(true, Tag::EXPIRED_TIME, Tag::EXPIRED_TIME); // fake expired Tag
    boost::shared_ptr<TypedCacheData> cache_data = cache->load(&cache_ctx, tag);
    if (!cache_data.get() || tag.expired()) {
        return false;
    }

    TypedValue ret = TypedValue::createMapValue();
    ret.add(STR_KEY, cache_data->key());
    ret.add(STR_VALUE, cache_data->value());
    ret.add(STR_EXPIRED, (boost::uint64_t)tag.expire_time);

    ret.swap(result);
    return true;
}


struct ModuleRegisterer {
    ModuleRegisterer() {
        ExternalModulePtr m(new ExternalModule());

        m->registerFunction("save", &save, 3, 3);
        m->registerFunction("load", &load, 1, 1);
        m->registerFunction("loadData", &loadData, 1, 1);

        ExternalModules::instance()->registerModule("cache", m);
    }

};

static ModuleRegisterer reg_;


} // namespace module_cache

} // namespace xscript
