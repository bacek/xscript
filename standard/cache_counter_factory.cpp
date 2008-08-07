#include "xscript/cache_counter_impl.h"

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

static ComponentRegisterer<CacheCounterFactory> reg_(new CacheCounterFactoryImpl());
}

