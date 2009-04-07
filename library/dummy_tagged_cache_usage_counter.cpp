#include "settings.h"
#include "details/dummy_tagged_cache_usage_counter.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

void
DummyTaggedCacheUsageCounter::fetchedHit(const Context *ctx, const Object *obj) {
    (void)ctx;
    (void)obj;
}

void
DummyTaggedCacheUsageCounter::fetchedMiss(const Context *ctx, const Object *obj) {
    (void)ctx;
    (void)obj;
}

XmlNodeHelper
DummyTaggedCacheUsageCounter::createReport() const {
    return XmlNodeHelper(xmlNewNode(0, (const xmlChar*) "dummy"));
}

} // namespace xscript
