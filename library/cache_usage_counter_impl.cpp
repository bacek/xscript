#include "settings.h"

#include <boost/lexical_cast.hpp>
#include "internal/cache_usage_counter_impl.h"

namespace xscript {

CacheUsageCounterImpl::CacheUsageCounterImpl(const std::string& name)
        : CounterImpl(name), max_(0) {
    reset();
}

void
CacheUsageCounterImpl::fetched(const std::string& name) {
    boost::mutex::scoped_lock lock(mtx_);
    std::map<std::string, int>::iterator it = counter_.find(name);
    if (it != counter_.end()) {
        it->second++;
    }
}

void
CacheUsageCounterImpl::stored(const std::string& name) {
    boost::mutex::scoped_lock lock(mtx_);
    std::map<std::string, int>::iterator it = counter_.find(name);
    if (it == counter_.end()) {
        counter_.insert(std::make_pair(name, 0));
    }
}

void
CacheUsageCounterImpl::removed(const std::string& name) {
    boost::mutex::scoped_lock lock(mtx_);
    std::map<std::string, int>::iterator it = counter_.find(name);
    if (it != counter_.end()) {
        if (elements_ == std::numeric_limits<unsigned long long>::max()) {
            elements_ = 1;
            average_usage_ = static_cast<double>(it->second);
        }
        else {
            average_usage_ = (average_usage_*elements_ + it->second)/(elements_ + 1);
            elements_++;
        }
        counter_.erase(it);
    }
}

void
CacheUsageCounterImpl::reset() {
    counter_.clear();
    average_usage_ = 0.0;
    elements_ = 0;
}

void
CacheUsageCounterImpl::max(boost::uint64_t val) {
    max_ = val;
}

XmlNodeHelper
CacheUsageCounterImpl::createReport() const {

    XmlNodeHelper line(xmlNewNode(0, (const xmlChar*) name_.c_str()));

    boost::mutex::scoped_lock lock(mtx_);

    double average_usage = average_usage_;
    long long docs = elements_;

    for(std::map<std::string, int>::const_iterator it = counter_.begin();
        it != counter_.end();
        ++it) {
        average_usage = (average_usage*docs + it->second)/(docs + 1);
        docs++;
    }

    xmlSetProp(line.get(),
               (const xmlChar*) "usage",
               (const xmlChar*) boost::lexical_cast<std::string>(average_usage).c_str());
    xmlSetProp(line.get(),
               (const xmlChar*) "elements",
               (const xmlChar*) boost::lexical_cast<std::string>(counter_.size()).c_str());
    xmlSetProp(line.get(),
               (const xmlChar*) "max-elements",
               (const xmlChar*) boost::lexical_cast<std::string>(max_).c_str());

    return line;
}


}
