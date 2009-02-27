#include "settings.h"

#include "xscript/cache_usage_counter.h"
#include "internal/cache_usage_counter_impl.h"
#include "details/dummy_cache_usage_counter.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

CacheUsageCounterFactory::CacheUsageCounterFactory() {
} 

CacheUsageCounterFactory::~CacheUsageCounterFactory() {
}

void
CacheUsageCounterFactory::init(const Config *config) {
    (void)config;
}

std::auto_ptr<CacheUsageCounter>
CacheUsageCounterFactory::createCounter(const std::string& name, bool want_real) {
    if (want_real) {
        return std::auto_ptr<CacheUsageCounter>(new CacheUsageCounterImpl(name));
    }
    else {
        return std::auto_ptr<CacheUsageCounter>(new DummyCacheUsageCounter());
    }
}

static ComponentRegisterer<CacheUsageCounterFactory> reg_;

}
