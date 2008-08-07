#ifndef _XSCRIPT_DETAILS_DEFAULT_SIMPLE_COUNTER_FACTORY_H_
#define _XSCRIPT_DETAILS_DEFAULT_SIMPLE_COUNTER_FACTORY_H_

#include "xscript/simple_counter.h"

namespace xscript
{
    /**
     * Default SimpleCounterFactory.
     *
     * Creates Dummy or real SimpleCounter
     */
    class DefaultSimpleCounterFactory : public SimpleCounterFactory
    {
    public:
        DefaultSimpleCounterFactory();
        ~DefaultSimpleCounterFactory();

        virtual void init(const Config *config);

        std::auto_ptr<SimpleCounter> createCounter(const std::string& name, bool want_real);
    };
}

#endif
