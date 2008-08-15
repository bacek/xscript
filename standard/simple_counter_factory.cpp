#include "internal/simple_counter_impl.h"

namespace xscript
{

class SimpleCounterFactoryImpl : public SimpleCounterFactory {
public:
    virtual std::auto_ptr<SimpleCounter> createCounter(const std::string& name, bool want_real);
};

std::auto_ptr<SimpleCounter> 
SimpleCounterFactoryImpl::createCounter(const std::string &name, bool want_real) {
    (void)want_real;
    return std::auto_ptr<SimpleCounter>(new SimpleCounterImpl(name));
}

static ComponentRegisterer<SimpleCounterFactory> reg_(new SimpleCounterFactoryImpl());
}


