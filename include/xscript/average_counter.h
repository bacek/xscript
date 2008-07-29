#ifndef _XSCRIPT_LINE_COUNTER_H_
#define _XSCRIPT_LINE_COUNTER_H_

#include <boost/thread/mutex.hpp>
#include <boost/cstdint.hpp>
#include <xscript/counter_base.h>
#include <xscript/component.h>

namespace xscript
{
	/**
	 * Time statistic gatherer. min/max/avg time for something.
	 */
	class AverageCounter : public CounterBase
	{
	public:
		/**
		 * Add single measure.
		 */
		virtual void add(uint64_t value) = 0;

		/**
		 * Remove single measure.
		 */
		virtual void remove(uint64_t value) = 0;
	};

    class AverageCounterFactory : public Component<AverageCounterFactory> {
    public:

        friend class ComponentRegisterer<AverageCounterFactory>;

        virtual std::auto_ptr<AverageCounter> createCounter(const std::string& name) = 0;
    };
}

#endif
