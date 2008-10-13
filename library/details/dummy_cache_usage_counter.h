#ifndef _XSCRIPT_DETAILS_DUMMY_CACHE_USAGE_COUNTER_H
#define _XSCRIPT_DETAILS_DUMMY_CACHE_USAGE_COUNTER_H

#include "xscript/cache_usage_counter.h"

namespace xscript {
/**
 * Do nothing counter
 */
class DummyCacheUsageCounter : public CacheUsageCounter {
public:
    void fetched(const std::string& name);
    void stored(const std::string& name);
    void removed(const std::string& name);
    void reset();
    void max(boost::uint64_t val);

    virtual XmlNodeHelper createReport() const;
};


}

#endif



