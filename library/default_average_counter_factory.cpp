#include "settings.h"

#include "details/default_average_counter_factory.h"
#include "internal/average_counter_impl.h"
#include "details/dummy_average_counter.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

DefaultAverageCounterFactory::DefaultAverageCounterFactory() {
}

DefaultAverageCounterFactory::~DefaultAverageCounterFactory() {
}

void DefaultAverageCounterFactory::init(const Config *config) {
    (void)config;
}

std::auto_ptr<AverageCounter> DefaultAverageCounterFactory::createCounter(const std::string& name, bool want_real) {
    if (want_real)
        return std::auto_ptr<AverageCounter>(new AverageCounterImpl(name));
    else
        return std::auto_ptr<AverageCounter>(new DummyAverageCounter());
}

REGISTER_COMPONENT2(AverageCounterFactory, DefaultAverageCounterFactory);
static ComponentRegisterer<AverageCounterFactory> reg_(new DefaultAverageCounterFactory());

}
