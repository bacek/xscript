#ifndef _XSCRIPT_COUNTER_BASE_H
#define _XSCRIPT_COUNTER_BASE_H

#include <string>
#include <libxml/tree.h>

#include <xscript/xml_helpers.h>

namespace xscript
{

	/**
	 * Base class for statistics counters
	 */
	class CounterBase
	{
	public:
		CounterBase(const std::string& name)
			: name_(name)
		{
		}

		virtual ~CounterBase() {};

		/**
		 * Create single xmlNode for statistic report. 
		 * E.g. <cache_miss total="1000" min_time="1" max_time="100" avg_time="25"/>
		 */
		virtual XmlNodeHelper createReport() const = 0;

	protected:
		std::string name_;
	};
}

#endif
