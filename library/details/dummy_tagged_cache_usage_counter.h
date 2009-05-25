#ifndef _XSCRIPT_DETAILS_DUMMY_TAGGED_CACHE_USAGE_COUNTER_H
#define _XSCRIPT_DETAILS_DUMMY_TAGGED_CACHE_USAGE_COUNTER_H

#include "xscript/tagged_cache_usage_counter.h"

namespace xscript {
/**
 * Do nothing counter
 */
class DummyTaggedCacheUsageCounter : public TaggedCacheUsageCounter {
public:    
    void fetchedHit(const Context *ctx, const Object *obj, const TagKey *key);
    void fetchedMiss(const Context *ctx, const Object *obj, const TagKey *key);
    virtual XmlNodeHelper createReport() const;
};

}

#endif



