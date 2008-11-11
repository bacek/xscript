#include "settings.h"
#include "details/dummy_cache_usage_counter.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

void
DummyCacheUsageCounter::fetched(const std::string& name) {
    (void)name;
}

void
DummyCacheUsageCounter::stored(const std::string& name) {
    (void)name;
}

void
DummyCacheUsageCounter::removed(const std::string& name) {
    (void)name;
}

XmlNodeHelper
DummyCacheUsageCounter::createReport() const {
    return XmlNodeHelper(xmlNewNode(0, (const xmlChar*) "dummy"));
}

void
DummyCacheUsageCounter::reset() {
}

void
DummyCacheUsageCounter::max(boost::uint64_t val) {
    (void)val;
}

} // namespace xscript
