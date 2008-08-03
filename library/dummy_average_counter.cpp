#include "details/dummy_average_counter.h"

namespace xscript
{

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
	return XmlNodeHelper(xmlNewNode(0, BAD_CAST "dummy"));
}

DummyAverageCounterFactory::DummyAverageCounterFactory() {
}

DummyAverageCounterFactory::~DummyAverageCounterFactory() {
}

void DummyAverageCounterFactory::init(const Config *config) {
    (void)config;
}

std::auto_ptr<AverageCounter> DummyAverageCounterFactory::createCounter(const std::string& name) {
    (void)name;
    return std::auto_ptr<AverageCounter>(new DummyAverageCounter());
}

REGISTER_COMPONENT2(AverageCounterFactory, DummyAverageCounterFactory);
static ComponentRegisterer<AverageCounterFactory> reg_(new DummyAverageCounterFactory());

}
