#include "settings.h"

#include <boost/lexical_cast.hpp>
#include "uptime_counter.h"

namespace xscript {

UptimeCounter::UptimeCounter() {
    time(&startTime_);
}

XmlNodeHelper UptimeCounter::createReport() const {
    time_t currentTime;
    time(&currentTime);

    time_t delta = currentTime - startTime_;

    XmlNodeHelper line(xmlNewNode(0, (const xmlChar*) "uptime"));

    xmlSetProp(line.get(), (const xmlChar*) "uptime", (const xmlChar*) boost::lexical_cast<std::string>(delta).c_str());

    xmlSetProp(line.get(), (const xmlChar*) "days", (const xmlChar*) boost::lexical_cast<std::string>(delta / 86400).c_str());
    delta %= 86400;
    xmlSetProp(line.get(), (const xmlChar*) "hours", (const xmlChar*) boost::lexical_cast<std::string>(delta / 3600).c_str());
    delta %= 3600;
    xmlSetProp(line.get(), (const xmlChar*) "mins", (const xmlChar*) boost::lexical_cast<std::string>(delta / 60).c_str());
    xmlSetProp(line.get(), (const xmlChar*) "secs", (const xmlChar*) boost::lexical_cast<std::string>(delta % 60).c_str());

    return line;
}


}
