#include <boost/lexical_cast.hpp>
#include "simple_counter_impl.h"

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
	XmlNodeHelper line(xmlNewNode(0, BAD_CAST name_.c_str()));

    boost::mutex::scoped_lock lock(mtx_);
	xmlSetProp(line.get(), BAD_CAST "count", BAD_CAST boost::lexical_cast<std::string>(count_).c_str());
	xmlSetProp(line.get(), BAD_CAST "peak", BAD_CAST boost::lexical_cast<std::string>(peak_).c_str());

	return line;
}

class SimpleCounterFactoryImpl : public SimpleCounterFactory {
public:
    virtual std::auto_ptr<SimpleCounter> createCounter(const std::string& name);
};

std::auto_ptr<SimpleCounter> 
SimpleCounterFactoryImpl::createCounter(const std::string &name) {
    return std::auto_ptr<SimpleCounter>(new SimpleCounterImpl(name));
}

static ComponentRegisterer<SimpleCounterFactory> reg_(new SimpleCounterFactoryImpl());

}
