#include "settings.h"

#include <boost/lexical_cast.hpp>

#include "xscript/average_counter.h"

#include "internal/cache_counter_impl.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

CacheCounterImpl::CacheCounterImpl(const std::string& name)
        : CounterImpl(name), stored_(0), loaded_(0), expired_(0), excluded_(0)
{}

// FIXME We should use atomic_increment in this methods
void
CacheCounterImpl::incUsedMemory(size_t amount) {
    (void)amount;
}

void
CacheCounterImpl::decUsedMemory(size_t amount) {
    (void)amount;
}

void
CacheCounterImpl::incLoaded() {
    boost::mutex::scoped_lock lock(mtx_);
    ++loaded_;
}

void
CacheCounterImpl::incStored() {
    boost::mutex::scoped_lock lock(mtx_);
    ++stored_;
}

void
CacheCounterImpl::incExpired() {
    boost::mutex::scoped_lock lock(mtx_);
    ++expired_;
}

void
CacheCounterImpl::incExcluded() {
    boost::mutex::scoped_lock lock(mtx_);
    ++excluded_;
}

XmlNodeHelper CacheCounterImpl::createReport() const {
    XmlNodeHelper report(xmlNewNode(0, (const xmlChar*) name_.c_str()));

    boost::mutex::scoped_lock lock(mtx_);
    xmlSetProp(report.get(), (const xmlChar*) "stored",
            (const xmlChar*) boost::lexical_cast<std::string>(stored_).c_str());
    xmlSetProp(report.get(), (const xmlChar*) "loaded",
            (const xmlChar*) boost::lexical_cast<std::string>(loaded_).c_str());
    xmlSetProp(report.get(), (const xmlChar*) "expired",
            (const xmlChar*) boost::lexical_cast<std::string>(expired_).c_str());
    xmlSetProp(report.get(), (const xmlChar*) "excluded",
            (const xmlChar*) boost::lexical_cast<std::string>(excluded_).c_str());
    
    return report;
}


}
