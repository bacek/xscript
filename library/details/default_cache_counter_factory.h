#ifndef _XSCRIPT_DETAILS_DEFAULT_CACHE_COUNTER_FACTORY_H
#define _XSCRIPT_DETAILS_DEFAULT_CACHE_COUNTER_FACTORY_H


#include "settings.h"
#include "xscript/cache_counter.h"

namespace xscript
{
    /**
     * Default SimpleCounterFactory.
     *
     * Creates Dummy or real CacheCounter
     */
    class DefaultCacheCounterFactory : public CacheCounterFactory {
    public:
        DefaultCacheCounterFactory();
        ~DefaultCacheCounterFactory();

        virtual void init(const Config *config);

        std::auto_ptr<CacheCounter> createCounter(const std::string& name, bool want_real);
    };

}

#endif



