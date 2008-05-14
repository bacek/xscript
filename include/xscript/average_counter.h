#ifndef _XSCRIPT_LINE_COUNTER_H_
#define _XSCRIPT_LINE_COUNTER_H_

#include <xscript/counter_base.h>
#include <boost/thread/mutex.hpp>
#include <boost/cstdint.hpp>

namespace xscript
{
	/**
	 * Time statistic gatherer. min/max/avg time for call.
	 */
	class AverageCounter : public CounterBase
	{
	public:
		AverageCounter(const std::string& name);
		~AverageCounter();

		/**
		 * Add single measure.
		 */
		void add(uint64_t value);

		/**
		 * Remove single measure.
		 */
		void remove(uint64_t value);

		virtual XmlNodeHelper createReport() const;

	protected:
		boost::mutex mtx_;
		uint64_t count_;
		uint64_t total_;
		uint64_t max_;
		uint64_t min_;
	};
}

#endif
