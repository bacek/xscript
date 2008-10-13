#include "settings.h"

#include "internal/cache_usage_counter_impl.h"

namespace xscript
{

class CacheUsageCounterFactoryImpl : public CacheUsageCounterFactory {
public:
    virtual std::auto_ptr<CacheUsageCounter> createCounter(const std::string& name, bool want_real);
};

std::auto_ptr<CacheUsageCounter> 
CacheUsageCounterFactoryImpl::createCounter(const std::string &name, bool want_real) {
    (void)want_real;
    return std::auto_ptr<CacheUsageCounter>(new CacheUsageCounterImpl(name));
}

static ComponentRegisterer<CacheUsageCounterFactory> reg_(new CacheUsageCounterFactoryImpl());
}

