#include "settings.h"

#include "xscript/config.h"
#include "xscript/tagged_cache_usage_counter.h"
#include "internal/tagged_cache_usage_counter_impl.h"
#include "details/dummy_tagged_cache_usage_counter.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

double TaggedCacheUsageCounterFactory::hit_ratio_level_;
unsigned int TaggedCacheUsageCounterFactory::output_size_;
time_t TaggedCacheUsageCounterFactory::store_time_;

TaggedCacheUsageCounterFactory::TaggedCacheUsageCounterFactory() {
} 

TaggedCacheUsageCounterFactory::~TaggedCacheUsageCounterFactory() {
}

void
TaggedCacheUsageCounterFactory::init(const Config *config) {
    output_size_ = config->as<unsigned int>("/xscript/statistics/tagged-cache/output-size", 10);
    hit_ratio_level_ = config->as<double>("/xscript/statistics/tagged-cache/hit-ratio-level", 0.3);
    store_time_ = config->as<time_t>("/xscript/statistics/tagged-cache/store-time", 600);
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

unsigned int
TaggedCacheUsageCounterFactory::outputSize() {
    return output_size_;
}

double
TaggedCacheUsageCounterFactory::hitRatioLevel() {
    return hit_ratio_level_;
}

time_t
TaggedCacheUsageCounterFactory::storeTime() {
    return store_time_;
}

static ComponentRegisterer<TaggedCacheUsageCounterFactory> reg_;

}
