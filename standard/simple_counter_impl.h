#ifndef _XSCRIPT_SIMPLE_COUNTER_IMPL_H_
#define _XSCRIPT_SIMPLE_COUNTER_IMPL_H_

#include <boost/thread/mutex.hpp>
#include <boost/cstdint.hpp>
#include <xscript/simple_counter.h>
#include "counter_impl.h"

namespace xscript
{
	class SimpleCounterImpl : public SimpleCounter, private CounterImpl
	{
	public:
		SimpleCounterImpl(const std::string& name);

		virtual XmlNodeHelper createReport() const;

		void inc();
		void dec();

	private:
		uint64_t count_;
		uint64_t peak_;
	};

    /*
    class SimpleCounterFactoryImpl : public Component<SimpleCounterFactory>
    {
    public:
        SimpleCounterFactoryImpl();
        ~SimpleCounterFactoryImpl();

        friend class ComponentRegisterer<SimpleCounterFactory>;

        virtual void init(const Config *config);

        std::auto_ptr<SimpleCounter> createCounter(const std::string& name);
    };
    */
}

#endif
