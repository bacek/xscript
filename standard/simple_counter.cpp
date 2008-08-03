#include <boost/lexical_cast.hpp>
#include "xscript/simple_counter.h"

namespace xscript
{

SimpleCounter::SimpleCounter(const std::string& name)
	: CounterBase(name), count_(0), peak_(0)
{
}

void SimpleCounter::inc() {
	boost::mutex::scoped_lock l(mtx_);
	++count_;
	peak_ = std::max(peak_, count_);
}

void SimpleCounter::dec() {
	boost::mutex::scoped_lock l(mtx_);
	--count_;
}

XmlNodeHelper SimpleCounter::createReport() const {
	boost::mutex::scoped_lock l(mtx_);

	XmlNodeHelper line(xmlNewNode(0, BAD_CAST name_.c_str()));

	xmlSetProp(line.get(), BAD_CAST "count", BAD_CAST boost::lexical_cast<std::string>(count_).c_str());
	xmlSetProp(line.get(), BAD_CAST "peak", BAD_CAST boost::lexical_cast<std::string>(peak_).c_str());

	return line;
}


}
