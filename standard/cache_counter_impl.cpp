#include <boost/lexical_cast.hpp>
#include "cache_counter_impl.h"

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

    XmlNodeHelper line(xmlNewNode(0, BAD_CAST name_.c_str()));

    boost::mutex::scoped_lock lock(mtx_);
    xmlSetProp(line.get(), BAD_CAST "used-memory", BAD_CAST boost::lexical_cast<std::string>(usedMemory_).c_str());
    xmlSetProp(line.get(), BAD_CAST "stored", BAD_CAST boost::lexical_cast<std::string>(stored_).c_str());
    xmlSetProp(line.get(), BAD_CAST "loaded", BAD_CAST boost::lexical_cast<std::string>(loaded_).c_str());
    xmlSetProp(line.get(), BAD_CAST "removed", BAD_CAST boost::lexical_cast<std::string>(removed_).c_str());

    return line;
}



class CacheCounterFactoryImpl : public CacheCounterFactory {
public:
    virtual std::auto_ptr<CacheCounter> createCounter(const std::string& name);
};

std::auto_ptr<CacheCounter>
CacheCounterFactoryImpl::createCounter(const std::string &name) {
    return std::auto_ptr<CacheCounter>(new CacheCounterImpl(name));
}

static ComponentRegisterer<CacheCounterFactory> reg_(new CacheCounterFactoryImpl());

}
