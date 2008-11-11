#include "settings.h"

#include <limits>
#include <boost/lexical_cast.hpp>
#include "internal/average_counter_impl.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

AverageCounterImpl::AverageCounterImpl(const std::string& name)
        : CounterImpl(name),
        count_(0), total_(0), max_(0), min_(std::numeric_limits<uint64_t>::max()) {
}

AverageCounterImpl::~AverageCounterImpl() {
}

void AverageCounterImpl::add(uint64_t value) {
    if (value == 0) {
        return;
    }

    boost::mutex::scoped_lock lock(mtx_);

    count_++;
    total_ += value;
    max_ = std::max(max_, value);
    min_ = std::min(min_, value);
}

void AverageCounterImpl::remove(uint64_t value) {
    if (value == 0) {
        return;
    }

    boost::mutex::scoped_lock lock(mtx_);

    count_--;
    total_ -= value;
}

XmlNodeHelper AverageCounterImpl::createReport() const {
    XmlNodeHelper line(xmlNewNode(0, (const xmlChar*) name_.c_str()));

    boost::mutex::scoped_lock lock(mtx_);
    xmlSetProp(line.get(), (const xmlChar*) "count", (const xmlChar*) boost::lexical_cast<std::string>(count_).c_str());
    if (count_ != 0) {
        xmlSetProp(line.get(), (const xmlChar*) "total", (const xmlChar*) boost::lexical_cast<std::string>(total_).c_str());
        xmlSetProp(line.get(), (const xmlChar*) "min", (const xmlChar*) boost::lexical_cast<std::string>(min_).c_str());
        xmlSetProp(line.get(), (const xmlChar*) "max", (const xmlChar*) boost::lexical_cast<std::string>(max_).c_str());
        xmlSetProp(line.get(), (const xmlChar*) "avg", (const xmlChar*) boost::lexical_cast<std::string>(total_ / count_).c_str());
    }

    return line;
}

} // namespace xscript
