#ifndef _XSCRIPT_SIMPLE_COUNTER_H_
#define _XSCRIPT_SIMPLE_COUNTER_H_

#include <boost/thread/mutex.hpp>
#include <boost/cstdint.hpp>
#include <xscript/counter_base.h>
#include <xscript/component.h>

namespace xscript
{
	class SimpleCounter : public CounterBase
	{
	public:
		virtual void inc() = 0;
		virtual void dec() = 0;
		
        class ScopedCount
		{
		public:
			ScopedCount(SimpleCounter * counter)
				: counter_(counter)
			{
				counter_->inc();
			}
			~ScopedCount() {
				counter_->dec();
			}
		private:
			SimpleCounter * counter_;
		};

	};

    class SimpleCounterFactory : public Component<SimpleCounterFactory>
    {
    public:
        friend class ComponentRegisterer<SimpleCounterFactory>;

        virtual std::auto_ptr<SimpleCounter> createCounter(const std::string& name) = 0;
    };
}

#endif
