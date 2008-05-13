#ifndef _XSCRIPT_DAEMON_UPTIME_COUNTER_H_
#define _XSCRIPT_DAEMON_UPTIME_COUNTER_H_

#include <time.h>
#include "xscript/counter_base.h"

namespace xscript
{

	class UptimeCounter : public CounterBase
	{
	public:
		UptimeCounter(const std::string& name = "uptime-counter");

		/**
		 * Create single xmlNode for statistic report. 
		 * E.g. <cache_miss total="1000" min_time="1" max_time="100" avg_time="25"/>
		 */
		XmlNodeHelper createReport() const;

	private:
		time_t startTime_;

	};
}

#endif
