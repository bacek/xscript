#ifndef _XSCRIPT_DETAILS_DUMMY_TAGGED_CACHE_USAGE_COUNTER_H
#define _XSCRIPT_DETAILS_DUMMY_TAGGED_CACHE_USAGE_COUNTER_H

#include "xscript/tagged_cache_usage_counter.h"

namespace xscript {
/**
 * Do nothing counter
 */
class DummyTaggedCacheUsageCounter : public TaggedCacheUsageCounter {
public:    
    void fetchedHit(const TagKey *key, const TaggedBlock *block);
    void fetchedMiss(const TagKey *key, const TaggedBlock *block);
    virtual XmlNodeHelper createReport() const;
};

}

#endif



