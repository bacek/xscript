#ifndef _XSCRIPT_INTERNAL_CACHE_USAGE_COUNTER_IMPL_H
#define _XSCRIPT_INTERNAL_CACHE_USAGE_COUNTER_IMPL_H

#include <string>
#include <map>
#include "xscript/cache_usage_counter.h"
#include "internal/counter_impl.h"

namespace xscript {

/**
 * Counter for measure cache usage statistic.
 */
class CacheUsageCounterImpl : virtual public CacheUsageCounter, virtual private CounterImpl {
public:
    CacheUsageCounterImpl(const std::string& name);
    virtual XmlNodeHelper createReport() const;

    void fetched(const std::string& name);
    void stored(const std::string& name);
    void removed(const std::string& name);
    void reset();
    void max(boost::uint64_t val);

private:
    std::map<std::string, int> counter_;
    double average_usage_;
    unsigned long long elements_;
    boost::uint64_t max_;
};
}

#endif
