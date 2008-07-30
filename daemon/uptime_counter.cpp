#include <boost/lexical_cast.hpp>
#include "uptime_counter.h"

namespace xscript
{

UptimeCounter::UptimeCounter()
{
	time(&startTime_);
}
		
XmlNodeHelper UptimeCounter::createReport() const {
	time_t currentTime;
	time(&currentTime);

	time_t delta = currentTime - startTime_;

	XmlNodeHelper line(xmlNewNode(0, BAD_CAST "uptime"));

	xmlSetProp(line.get(), BAD_CAST "uptime", BAD_CAST boost::lexical_cast<std::string>(delta).c_str());

	xmlSetProp(line.get(), BAD_CAST "days", BAD_CAST boost::lexical_cast<std::string>(delta / 86400).c_str());
    delta %= 86400;
	xmlSetProp(line.get(), BAD_CAST "hours", BAD_CAST boost::lexical_cast<std::string>(delta / 3600).c_str());
    delta %= 3600;
	xmlSetProp(line.get(), BAD_CAST "mins", BAD_CAST boost::lexical_cast<std::string>(delta / 60).c_str());
	xmlSetProp(line.get(), BAD_CAST "secs", BAD_CAST boost::lexical_cast<std::string>(delta % 60).c_str());

	return line;
}


}
