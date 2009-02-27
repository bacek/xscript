#include "settings.h"

#include "xscript/cache_counter.h"
#include "internal/cache_counter_impl.h"
#include "details/dummy_cache_counter.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

CacheCounterFactory::CacheCounterFactory() { } 
CacheCounterFactory::~CacheCounterFactory() { }

void CacheCounterFactory::init(const Config *config) {
    (void)config;
}

std::auto_ptr<CacheCounter> CacheCounterFactory::createCounter(const std::string& name, bool want_real) {
    if (want_real)
        return std::auto_ptr<CacheCounter>(new CacheCounterImpl(name));
    else
        return std::auto_ptr<CacheCounter>(new DummyCacheCounter());
}

static ComponentRegisterer<CacheCounterFactory> reg_;

} // namespace xscript
