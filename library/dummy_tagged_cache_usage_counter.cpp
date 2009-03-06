#include "settings.h"
#include "details/dummy_tagged_cache_usage_counter.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

void
DummyTaggedCacheUsageCounter::fetched(const TagKey *key, const TaggedBlock *block) {
    (void)key;
    (void)block;
}

XmlNodeHelper
DummyTaggedCacheUsageCounter::createReport() const {
    return XmlNodeHelper(xmlNewNode(0, (const xmlChar*) "dummy"));
}

} // namespace xscript
