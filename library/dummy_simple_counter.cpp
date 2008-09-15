#include "settings.h"

#include "details/dummy_simple_counter.h"

namespace xscript {

DummySimpleCounter::DummySimpleCounter() { }
DummySimpleCounter::~DummySimpleCounter() { }
void DummySimpleCounter::inc() { }
void DummySimpleCounter::dec() { }


XmlNodeHelper DummySimpleCounter::createReport() const {
    return XmlNodeHelper(xmlNewNode(0, (const xmlChar*) "dummy"));
}

}
