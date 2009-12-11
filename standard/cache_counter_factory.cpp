#include "settings.h"

#include "internal/cache_counter_impl.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

class CacheCounterFactoryImpl : public CacheCounterFactory {
public:
    virtual std::auto_ptr<CacheCounter> createCounter(const std::string& name, bool want_real);
};

std::auto_ptr<CacheCounter> 
CacheCounterFactoryImpl::createCounter(const std::string &name, bool want_real) {
    (void)want_real;
    return std::auto_ptr<CacheCounter>(new CacheCounterImpl(name));
}

static ComponentImplRegisterer<CacheCounterFactory> reg_(new CacheCounterFactoryImpl());
}

