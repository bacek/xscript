#include "settings.h"

#include "details/dummy_average_counter.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

DummyAverageCounter::DummyAverageCounter() {
}

DummyAverageCounter::~DummyAverageCounter() {
}

void DummyAverageCounter::add(uint64_t value) {
    (void)value;
}

void DummyAverageCounter::remove(uint64_t value) {
    (void)value;
}

boost::uint64_t
DummyAverageCounter::count() const {
    return 0;
}

XmlNodeHelper DummyAverageCounter::createReport() const {
    return XmlNodeHelper(xmlNewNode(0, (const xmlChar*) "dummy"));
}

} // namespace xscript
