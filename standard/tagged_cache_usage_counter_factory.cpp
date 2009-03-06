#include "settings.h"

#include "internal/tagged_cache_usage_counter_impl.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

class TaggedCacheUsageCounterFactoryImpl : public TaggedCacheUsageCounterFactory {
public:
    virtual std::auto_ptr<TaggedCacheUsageCounter> createCounter(const std::string& name, bool want_real);
};

std::auto_ptr<TaggedCacheUsageCounter> 
TaggedCacheUsageCounterFactoryImpl::createCounter(const std::string &name, bool want_real) {
    (void)want_real;
    return std::auto_ptr<TaggedCacheUsageCounter>(new TaggedCacheUsageCounterImpl(name));
}

static ComponentImplRegisterer<TaggedCacheUsageCounterFactory> reg_(new TaggedCacheUsageCounterFactoryImpl());
}

