#include "xscript/average_counter_impl.h"

namespace xscript
{

class AverageCounterFactoryImpl : public AverageCounterFactory {
public:
    virtual std::auto_ptr<AverageCounter> createCounter(const std::string& name, bool want_real);
};

std::auto_ptr<AverageCounter> 
AverageCounterFactoryImpl::createCounter(const std::string &name, bool want_real) {
    (void)want_real;
    return std::auto_ptr<AverageCounter>(new AverageCounterImpl(name));
}

static ComponentRegisterer<AverageCounterFactory> reg_(new AverageCounterFactoryImpl());
}

