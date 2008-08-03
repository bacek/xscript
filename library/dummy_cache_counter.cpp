#include <boost/lexical_cast.hpp>
#include "details/dummy_cache_counter.h"

namespace xscript
{

void DummyCacheCounter::incUsedMemory(size_t amount) {
    (void)amount;
}

void DummyCacheCounter::decUsedMemory(size_t amount) {
    (void)amount;
}


void DummyCacheCounter::incLoaded() { } 
void DummyCacheCounter::incStored() { }
void DummyCacheCounter::incRemoved() { }


XmlNodeHelper DummyCacheCounter::createReport() const {
	return XmlNodeHelper(xmlNewNode(0, BAD_CAST "dummy"));
}


DummyCacheCounterFactory::DummyCacheCounterFactory() { }
DummyCacheCounterFactory::~DummyCacheCounterFactory() { }

void DummyCacheCounterFactory::init(const Config *config) {
    (void)config;
}

std::auto_ptr<CacheCounter>
DummyCacheCounterFactory::createCounter(const std::string& name) {
    (void)name;
    return std::auto_ptr<CacheCounter>(new DummyCacheCounter());
}

REGISTER_COMPONENT2(CacheCounterFactory, DummyCacheCounterFactory);

}
