#include "settings.h"

#include "xscript/config.h"
#include "xscript/response_time_counter.h"
#include "internal/response_time_counter_impl.h"
#include "details/dummy_response_time_counter.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

ResponseTimeCounterFactory::ResponseTimeCounterFactory() {
} 

ResponseTimeCounterFactory::~ResponseTimeCounterFactory() {
}

void
ResponseTimeCounterFactory::init(const Config *config) {
    (void)config;
}

std::auto_ptr<ResponseTimeCounter>
ResponseTimeCounterFactory::createCounter(const std::string& name, bool want_real) {
    if (want_real) {
        return std::auto_ptr<ResponseTimeCounter>(new ResponseTimeCounterImpl(name));
    }
    else {
        return std::auto_ptr<ResponseTimeCounter>(new DummyResponseTimeCounter());
    }
}

static ComponentRegisterer<ResponseTimeCounterFactory> reg_;

}
