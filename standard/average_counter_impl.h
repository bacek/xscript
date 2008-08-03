#ifndef _XSCRIPT_STANDART_AVERAGE_COUNTER_IMPL_H
#define _XSCRIPT_STANDART_AVERAGE_COUNTER_IMPL_H

#include <boost/cstdint.hpp>
#include <xscript/average_counter.h>
#include "counter_impl.h"

namespace xscript
{
	/**
	 * Time statistic gatherer. min/max/avg time for call.
	 */
	class AverageCounterImpl : public AverageCounter, private CounterImpl
	{
	public:
		AverageCounterImpl(const std::string& name);
		~AverageCounterImpl();

		/**
		 * Add single measure.
		 */
		virtual void add(uint64_t value) = 0;

		/**
		 * Remove single measure.
		 */
		virtual void remove(uint64_t value) = 0;

		virtual XmlNodeHelper createReport() const;

	protected:
		uint64_t        count_;
		uint64_t        total_;
		uint64_t        max_;
		uint64_t        min_;
	};
}

#endif
