#include <boost/lexical_cast.hpp>
#include "xscript/cache_counter.h"

namespace xscript
{

CacheCounter::CacheCounter(const std::string& name)
	: name_(name), usedMemory_(0), stored_(0), loaded_(0), removed_(0)
{
}

XmlNodeHelper CacheCounter::createReport() const {
	XmlNodeHelper line(xmlNewNode(0, BAD_CAST name_.c_str()));

	xmlSetProp(line.get(), BAD_CAST "used-memory", BAD_CAST boost::lexical_cast<std::string>(usedMemory_).c_str());
	xmlSetProp(line.get(), BAD_CAST "stored", BAD_CAST boost::lexical_cast<std::string>(stored_).c_str());
	xmlSetProp(line.get(), BAD_CAST "loaded", BAD_CAST boost::lexical_cast<std::string>(loaded_).c_str());
	xmlSetProp(line.get(), BAD_CAST "removed", BAD_CAST boost::lexical_cast<std::string>(removed_).c_str());

	return line;
}

}
