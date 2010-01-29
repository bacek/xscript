#ifndef _XSCRIPT_CACHE_COUNTER_H_
#define _XSCRIPT_CACHE_COUNTER_H_

#include <string>
#include <cstddef>

#include <boost/cstdint.hpp>

#include <xscript/counter_base.h>
#include <xscript/component.h>

namespace xscript {

/**
 * Counter for measure cache statistic.
 * Trivial set of (loaded, stored, removed) counters.
 */
class CacheCounter : public CounterBase {
public:
    virtual void incUsedMemory(std::size_t amount) = 0;
    virtual void decUsedMemory(std::size_t amount) = 0;

    virtual void incLoaded() = 0;
    virtual void incInserted() = 0;
    virtual void incUpdated() = 0;
    virtual void incExcluded() = 0;
    virtual void incExpired() = 0;
};


class CacheCounterFactory : public Component<CacheCounterFactory> {
public:
    CacheCounterFactory();
    ~CacheCounterFactory();

    virtual void init(const Config *config);    
    virtual std::auto_ptr<CacheCounter> createCounter(const std::string& name, bool want_real = false);
};

} // namespace xscript

#endif // _XSCRIPT_CACHE_COUNTER_H_
    
