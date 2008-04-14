#ifndef _XSCRIPT_SIMPLE_COUNTER_H_
#define _XSCRIPT_SIMPLE_COUNTER_H_

#include <stdint.h>
#include <boost/thread/mutex.hpp>
#include <xscript/counter_base.h>

namespace xscript
{
	class SimpleCounter : public CounterBase
	{
	public:
		SimpleCounter(const std::string& name);

		virtual XmlNodeHelper createReport() const;

		void inc();
		void dec();

		class ScopedCount
		{
		public:
			ScopedCount(SimpleCounter & counter)
				: counter_(counter)
			{
				counter_.inc();
			}
			~ScopedCount() {
				counter_.dec();
			}
		private:
			SimpleCounter &counter_;
		};

	private:
		// Counter lock. Defined as mutable to use in 'createReport' method
		mutable boost::mutex mtx_;

		uint64_t count_;
		uint64_t peak_;
	};
}

#endif
