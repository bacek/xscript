#ifndef _XSCRIPT_SIMPLE_COUNTER_H_
#define _XSCRIPT_SIMPLE_COUNTER_H_

#include <boost/thread/mutex.hpp>
#include <xscript/counter_base.h>
#include <boost/cstdint.hpp>

namespace xscript
{
	class SimpleCounter : public CounterBase
	{
	public:
		SimpleCounter(const std::string& name);

		virtual XmlNodeHelper createReport() const;

		void inc();
		void dec();

	private:
		// Counter lock. Defined as mutable to use in 'createReport' method
		mutable boost::mutex mtx_;

		uint64_t count_;
		uint64_t peak_;
	};

    class SimpleCounterFactory : public Component<SimpleCounterFactory>
    {
    public:
        SimpleCounterFactory();
        ~SimpleCounterFactory();

        friend class ComponentRegisterer<SimpleCounterFactory>;

        virtual void init(const Config *config);

        std::auto_ptr<SimpleCounter> createCounter(const std::string& name);
    }
}

#endif
