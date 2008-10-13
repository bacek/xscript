#ifndef _XSCRIPT_DETAILS_DEFAULT_CACHE_USAGE_COUNTER_FACTORY_H
#define _XSCRIPT_DETAILS_DEFAULT_CACHE_USAGE_COUNTER_FACTORY_H

#include "xscript/cache_usage_counter.h"

namespace xscript
{
    /**
     * Default SimpleCounterFactory.
     *
     * Creates Dummy or real CacheUsageCounter
     */
    class DefaultCacheUsageCounterFactory : public CacheUsageCounterFactory {
    public:
        DefaultCacheUsageCounterFactory();
        ~DefaultCacheUsageCounterFactory();

        virtual void init(const Config *config);

        std::auto_ptr<CacheUsageCounter> createCounter(const std::string& name, bool want_real);
    };

}

#endif



