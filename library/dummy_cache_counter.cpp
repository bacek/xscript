#include "settings.h"

#include <boost/lexical_cast.hpp>
#include "details/dummy_cache_counter.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

void
DummyCacheCounter::incUsedMemory(size_t amount) {
    (void)amount;
}

void
DummyCacheCounter::decUsedMemory(size_t amount) {
    (void)amount;
}

void
DummyCacheCounter::incLoaded() {
}

void
DummyCacheCounter::incInserted() {
}

void
DummyCacheCounter::incUpdated() {
}

void 
DummyCacheCounter::incExpired() {
}

void
DummyCacheCounter::incExcluded() {
}

XmlNodeHelper
DummyCacheCounter::createReport() const {
    return XmlNodeHelper(xmlNewNode(0, (const xmlChar*) "dummy"));
}

} // namespace xscript
