#include "details/dummy_simple_counter.h"

namespace xscript {

DummySimpleCounter::DummySimpleCounter() { }
DummySimpleCounter::~DummySimpleCounter() { }
void DummySimpleCounter::inc() { }
void DummySimpleCounter::dec() { }


XmlNodeHelper DummySimpleCounter::createReport() const {
    return XmlNodeHelper(xmlNewNode(0, BAD_CAST "dummy"));
}

DummySimpleCounterFactory::DummySimpleCounterFactory() { }
DummySimpleCounterFactory::~DummySimpleCounterFactory() { }

void DummySimpleCounterFactory::init(const Config *config) {
    (void)config;
}

std::auto_ptr<SimpleCounter> DummySimpleCounterFactory::createCounter(const std::string& name) {
    (void)name;
    return std::auto_ptr<SimpleCounter>(new DummySimpleCounter());
}

REGISTER_COMPONENT2(SimpleCounterFactory, DummySimpleCounterFactory);
static ComponentRegisterer<SimpleCounterFactory> reg_(new DummySimpleCounterFactory());

}
