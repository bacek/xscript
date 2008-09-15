#include "settings.h"

#include <boost/lexical_cast.hpp>
#include "details/dummy_cache_counter.h"

namespace xscript {

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
    return XmlNodeHelper(xmlNewNode(0, (const xmlChar*) "dummy"));
}


}
