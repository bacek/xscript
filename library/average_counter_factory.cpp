#include "settings.h"

#include "xscript/average_counter.h"
#include "internal/average_counter_impl.h"
#include "details/dummy_average_counter.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

AverageCounterFactory::AverageCounterFactory() {
}

AverageCounterFactory::~AverageCounterFactory() {
}

void
AverageCounterFactory::init(const Config *config) {
    (void)config;
}

std::auto_ptr<AverageCounter>
AverageCounterFactory::createCounter(const std::string& name, bool want_real) {
    if (want_real)
        return std::auto_ptr<AverageCounter>(new AverageCounterImpl(name));
    else
        return std::auto_ptr<AverageCounter>(new DummyAverageCounter());
}

static ComponentRegisterer<AverageCounterFactory> reg_;

}
