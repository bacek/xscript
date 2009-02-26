#include "settings.h"

#include "details/default_cache_usage_counter_factory.h"
#include "internal/cache_usage_counter_impl.h"
#include "details/dummy_cache_usage_counter.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

DefaultCacheUsageCounterFactory::DefaultCacheUsageCounterFactory() {
} 

DefaultCacheUsageCounterFactory::~DefaultCacheUsageCounterFactory() {
}

void
DefaultCacheUsageCounterFactory::init(const Config *config) {
    (void)config;
}

std::auto_ptr<CacheUsageCounter>
DefaultCacheUsageCounterFactory::createCounter(const std::string& name, bool want_real) {
    if (want_real) {
        return std::auto_ptr<CacheUsageCounter>(new CacheUsageCounterImpl(name));
    }
    else {
        return std::auto_ptr<CacheUsageCounter>(new DummyCacheUsageCounter());
    }
}

REGISTER_COMPONENT2(CacheUsageCounterFactory, DefaultCacheUsageCounterFactory);
static ComponentRegisterer<CacheUsageCounterFactory> reg_(new DefaultCacheUsageCounterFactory());

}
