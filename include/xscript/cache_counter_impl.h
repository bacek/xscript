#ifndef _XSCRIPT_CACHE_COUNTER_IMPL_H
#define _XSCRIPT_CACHE_COUNTER_IMPL_H

#include <string>
#include <xscript/cache_counter.h>
#include <xscript/counter_impl.h>

namespace xscript {

/**
 * Counter for measure cache statistic.
 * Trivial set of (loaded, stored, removed) counters.
 */
class CacheCounterImpl : public CacheCounter, private CounterImpl {
public:
    CacheCounterImpl(const std::string& name);
    virtual XmlNodeHelper createReport() const;

    void incUsedMemory(size_t amount);
    void decUsedMemory(size_t amount);

    void incLoaded();
    void incStored();
    void incRemoved();

private:
    size_t usedMemory_;
    size_t stored_;
    size_t loaded_;
    size_t removed_;
};
}

#endif
