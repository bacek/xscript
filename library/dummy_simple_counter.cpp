#include "settings.h"

#include "details/dummy_simple_counter.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

DummySimpleCounter::DummySimpleCounter() { }
DummySimpleCounter::~DummySimpleCounter() { }
void DummySimpleCounter::inc() { }
void DummySimpleCounter::dec() { }
void DummySimpleCounter::max(uint64_t val) { (void)val; }


XmlNodeHelper DummySimpleCounter::createReport() const {
    return XmlNodeHelper(xmlNewNode(0, (const xmlChar*) "dummy"));
}

} // namespace xscript
