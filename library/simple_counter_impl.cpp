#include <boost/lexical_cast.hpp>
#include "internal/simple_counter_impl.h"

namespace xscript
{

SimpleCounterImpl::SimpleCounterImpl(const std::string& name)
	: CounterImpl(name), count_(0), peak_(0)
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

XmlNodeHelper SimpleCounterImpl::createReport() const {
	XmlNodeHelper line(xmlNewNode(0, (const xmlChar*) name_.c_str()));

    boost::mutex::scoped_lock lock(mtx_);
	xmlSetProp(line.get(), (const xmlChar*) "count", (const xmlChar*) boost::lexical_cast<std::string>(count_).c_str());
	xmlSetProp(line.get(), (const xmlChar*) "peak", (const xmlChar*) boost::lexical_cast<std::string>(peak_).c_str());

	return line;
}

}
