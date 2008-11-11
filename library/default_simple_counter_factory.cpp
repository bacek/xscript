#include "settings.h"

#include "details/default_simple_counter_factory.h"
#include "internal/simple_counter_impl.h"
#include "details/dummy_simple_counter.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

DefaultSimpleCounterFactory::DefaultSimpleCounterFactory() { } 
DefaultSimpleCounterFactory::~DefaultSimpleCounterFactory() { }

void DefaultSimpleCounterFactory::init(const Config *config) {
    (void)config;
}

std::auto_ptr<SimpleCounter> DefaultSimpleCounterFactory::createCounter(const std::string& name, bool want_real) {
    if (want_real)
        return std::auto_ptr<SimpleCounter>(new SimpleCounterImpl(name));
    else
        return std::auto_ptr<SimpleCounter>(new DummySimpleCounter());
}

REGISTER_COMPONENT2(SimpleCounterFactory, DefaultSimpleCounterFactory);
static ComponentRegisterer<SimpleCounterFactory> reg_(new DefaultSimpleCounterFactory());

} // namespace xscript
