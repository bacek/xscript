#ifndef _XSCRIPT_COUNTER_BASE_H
#define _XSCRIPT_COUNTER_BASE_H

#include <string>
#include <libxml/tree.h>

#include <xscript/xml_helpers.h>

namespace xscript
{

	/**
	 * Base class for statistics counters.
     *
     * All counters should consist of many parts:
     * 1. Counter interface definition. E.g. IAverageCounter.
     * 2. Dummy implementation of counter inside "library". (If it this counter
     *    used in core).
     * 3. Real implementation in "standart" or somewhere else.
     * 4. Complimentary factories for this counters.
	 */
	class CounterBase
	{
	public:
		virtual ~CounterBase() {};

		/**
		 * Create single xmlNode for statistic report. 
		 * E.g. <cache_miss total="1000" min_time="1" max_time="100" avg_time="25"/>
		 */
		virtual XmlNodeHelper createReport() const = 0;
	};
}

#endif
