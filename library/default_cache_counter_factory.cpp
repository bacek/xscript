#include "settings.h"

#include "details/default_cache_counter_factory.h"
#include "internal/cache_counter_impl.h"
#include "details/dummy_cache_counter.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

DefaultCacheCounterFactory::DefaultCacheCounterFactory() { } 
DefaultCacheCounterFactory::~DefaultCacheCounterFactory() { }

void DefaultCacheCounterFactory::init(const Config *config) {
    (void)config;
}

std::auto_ptr<CacheCounter> DefaultCacheCounterFactory::createCounter(const std::string& name, bool want_real) {
    if (want_real)
        return std::auto_ptr<CacheCounter>(new CacheCounterImpl(name));
    else
        return std::auto_ptr<CacheCounter>(new DummyCacheCounter());
}

REGISTER_COMPONENT2(CacheCounterFactory, DefaultCacheCounterFactory);
static ComponentRegisterer<CacheCounterFactory> reg_(new DefaultCacheCounterFactory());

} // namespace xscript
