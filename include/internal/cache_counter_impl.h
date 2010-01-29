#ifndef _XSCRIPT_INTERNAL_CACHE_COUNTER_IMPL_H
#define _XSCRIPT_INTERNAL_CACHE_COUNTER_IMPL_H

#include <string>
#include "xscript/cache_counter.h"
#include "internal/counter_impl.h"

namespace xscript {

class AverageCounter;

/**
 * Counter for measure cache statistic.
 * Trivial set of (loaded, stored, removed) counters.
 */
class CacheCounterImpl : public CacheCounter, private CounterImpl {
public:
    CacheCounterImpl(const std::string &name);
    virtual XmlNodeHelper createReport() const;

    void incUsedMemory(size_t amount);
    void decUsedMemory(size_t amount);

    void incLoaded();
    void incInserted();
    void incUpdated();
    void incExcluded();
    void incExpired();

private:
    size_t inserted_;
    size_t updated_;
    size_t loaded_;
    size_t expired_;
    size_t excluded_;
};
}

#endif
