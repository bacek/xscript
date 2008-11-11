#include "settings.h"

#include <boost/lexical_cast.hpp>
#include "internal/simple_counter_impl.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

SimpleCounterImpl::SimpleCounterImpl(const std::string& name)
	: CounterImpl(name), count_(0), peak_(0), max_(0)
{
}

void SimpleCounterImpl::inc() {
	boost::mutex::scoped_lock lock(mtx_);
	++count_;
	peak_ = std::max(peak_, count_);
}

void SimpleCounterImpl::dec() {
	boost::mutex::scoped_lock lock(mtx_);
	--count_;
}

void SimpleCounterImpl::max(uint64_t val) {
    max_ = val;
}

XmlNodeHelper SimpleCounterImpl::createReport() const {
    XmlNodeHelper line(xmlNewNode(NULL, (const xmlChar*) name_.c_str()));

    boost::mutex::scoped_lock lock(mtx_);
    uint64_t count = count_;
    uint64_t peak = peak_;
    uint64_t max = max_;
    lock.unlock();
    xmlSetProp(line.get(), (const xmlChar*) "count", (const xmlChar*) boost::lexical_cast<std::string>(count).c_str());
    xmlSetProp(line.get(), (const xmlChar*) "peak", (const xmlChar*) boost::lexical_cast<std::string>(peak).c_str());
    if (max) {
        xmlSetProp(line.get(), (const xmlChar*) "max", (const xmlChar*) boost::lexical_cast<std::string>(max).c_str());
    }

	return line;
}

}
