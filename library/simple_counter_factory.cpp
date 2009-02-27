#include "settings.h"

#include "xscript/simple_counter.h"
#include "internal/simple_counter_impl.h"
#include "details/dummy_simple_counter.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

SimpleCounterFactory::SimpleCounterFactory() { } 
SimpleCounterFactory::~SimpleCounterFactory() { }

void
SimpleCounterFactory::init(const Config *config) {
    (void)config;
}

std::auto_ptr<SimpleCounter>
SimpleCounterFactory::createCounter(const std::string& name, bool want_real) {
    if (want_real)
        return std::auto_ptr<SimpleCounter>(new SimpleCounterImpl(name));
    else
        return std::auto_ptr<SimpleCounter>(new DummySimpleCounter());
}

static ComponentRegisterer<SimpleCounterFactory> reg_;

} // namespace xscript
