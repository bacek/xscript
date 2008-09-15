#include "settings.h"

#include "details/dummy_average_counter.h"

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

XmlNodeHelper DummyAverageCounter::createReport() const {
    return XmlNodeHelper(xmlNewNode(0, (const xmlChar*) "dummy"));
}


}
