#include <limits>
#include <boost/lexical_cast.hpp>
#include "xscript/average_counter.h"

namespace xscript
{

AverageCounter::AverageCounter(const std::string& name)
	: CounterBase(name),
		count_(0), total_(0), max_(0), min_(std::numeric_limits<uint64_t>::max())
{
}

AverageCounter::~AverageCounter()
{
}

void AverageCounter::add(uint64_t value) {
	if (value == 0) {
		return;
	}

	boost::mutex::scoped_lock lock(mtx_);

	count_++;
	total_ += value;
	max_ = std::max(max_, value);
	min_ = std::min(min_, value);
}

void AverageCounter::remove(uint64_t value) {
	if (value == 0) {
		return;
	}

	boost::mutex::scoped_lock lock(mtx_);

	count_--;
	total_ -= value;
}

XmlNodeHelper AverageCounter::createReport() const {
	XmlNodeHelper line(xmlNewNode(0, BAD_CAST name_.c_str()));

	xmlSetProp(line.get(), BAD_CAST "count", BAD_CAST boost::lexical_cast<std::string>(count_).c_str());
	if (count_ != 0) {
		xmlSetProp(line.get(), BAD_CAST "total", BAD_CAST boost::lexical_cast<std::string>(total_).c_str());
		xmlSetProp(line.get(), BAD_CAST "min", BAD_CAST boost::lexical_cast<std::string>(min_).c_str());
		xmlSetProp(line.get(), BAD_CAST "max", BAD_CAST boost::lexical_cast<std::string>(max_).c_str());
		xmlSetProp(line.get(), BAD_CAST "avg", BAD_CAST boost::lexical_cast<std::string>(total_ / count_).c_str());
	}

	return line;
}

}
