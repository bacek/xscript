#include <boost/lexical_cast.hpp>
#include "internal/cache_counter_impl.h"

namespace xscript {

CacheCounterImpl::CacheCounterImpl(const std::string& name)
        : CounterImpl(name), usedMemory_(0), stored_(0), loaded_(0), removed_(0) {
}

// FIXME We should use atomic_increment in this methods
void CacheCounterImpl::incUsedMemory(size_t amount) {
    boost::mutex::scoped_lock lock(mtx_);
    usedMemory_ += amount;
}
void CacheCounterImpl::decUsedMemory(size_t amount) {
    boost::mutex::scoped_lock lock(mtx_);
    usedMemory_ -= amount;
}

void CacheCounterImpl::incLoaded() {
    boost::mutex::scoped_lock lock(mtx_);
    ++loaded_;
}

void CacheCounterImpl::incStored() {
    boost::mutex::scoped_lock lock(mtx_);
    ++stored_;
}

void CacheCounterImpl::incRemoved() {
    boost::mutex::scoped_lock lock(mtx_);
    ++removed_;
}

XmlNodeHelper CacheCounterImpl::createReport() const {

    XmlNodeHelper line(xmlNewNode(0, (const xmlChar*) name_.c_str()));

    boost::mutex::scoped_lock lock(mtx_);
    xmlSetProp(line.get(), (const xmlChar*) "used-memory", (const xmlChar*) boost::lexical_cast<std::string>(usedMemory_).c_str());
    xmlSetProp(line.get(), (const xmlChar*) "stored", (const xmlChar*) boost::lexical_cast<std::string>(stored_).c_str());
    xmlSetProp(line.get(), (const xmlChar*) "loaded", (const xmlChar*) boost::lexical_cast<std::string>(loaded_).c_str());
    xmlSetProp(line.get(), (const xmlChar*) "removed", (const xmlChar*) boost::lexical_cast<std::string>(removed_).c_str());

    return line;
}


}
