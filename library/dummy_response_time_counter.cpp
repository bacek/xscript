#include "settings.h"

#include "details/dummy_response_time_counter.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

void
DummyResponseTimeCounter::add(const Response *resp, boost::uint64_t value) {
    (void)resp;
    (void)value;
}

void
DummyResponseTimeCounter::add(const Context *ctx, boost::uint64_t value) {
    (void)ctx;
    (void)value;
}

XmlNodeHelper
DummyResponseTimeCounter::createReport() const {
    return XmlNodeHelper(xmlNewNode(0, (const xmlChar*) "dummy"));
}

void
DummyResponseTimeCounter::reset() {
}

} // namespace xscript
