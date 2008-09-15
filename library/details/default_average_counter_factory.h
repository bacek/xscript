#ifndef _XSCRIPT_DETAILS_DEFAULT_AVERAGE_COUNTER_FACTORY_H
#define _XSCRIPT_DETAILS_DEFAULT_AVERAGE_COUNTER_FACTORY_H

#include "xscript/average_counter.h"

namespace xscript
{
    /**
     * Default SimpleCounterFactory.
     *
     * Creates Dummy or real AverageCounter
     */
    class DefaultAverageCounterFactory : public AverageCounterFactory {
    public:
        DefaultAverageCounterFactory();
        ~DefaultAverageCounterFactory();

        virtual void init(const Config *config);

        std::auto_ptr<AverageCounter> createCounter(const std::string& name, bool want_real);
    };

}

#endif


