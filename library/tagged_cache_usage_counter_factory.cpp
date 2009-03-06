#include "settings.h"

#include "xscript/tagged_cache_usage_counter.h"
#include "internal/tagged_cache_usage_counter_impl.h"
#include "details/dummy_tagged_cache_usage_counter.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

TaggedCacheUsageCounterFactory::TaggedCacheUsageCounterFactory() {
} 

TaggedCacheUsageCounterFactory::~TaggedCacheUsageCounterFactory() {
}

void
TaggedCacheUsageCounterFactory::init(const Config *config) {
    (void)config;
}

std::auto_ptr<TaggedCacheUsageCounter>
TaggedCacheUsageCounterFactory::createCounter(const std::string& name, bool want_real) {
    if (want_real) {
        return std::auto_ptr<TaggedCacheUsageCounter>(new TaggedCacheUsageCounterImpl(name));
    }
    else {
        return std::auto_ptr<TaggedCacheUsageCounter>(new DummyTaggedCacheUsageCounter());
    }
}

static ComponentRegisterer<TaggedCacheUsageCounterFactory> reg_;

}
