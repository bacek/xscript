#include "settings.h"

#include "internal/response_time_counter_impl.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

class ResponseTimeCounterFactoryImpl : public ResponseTimeCounterFactory {
public:
    virtual std::auto_ptr<ResponseTimeCounter> createCounter(const std::string& name, bool want_real);
};

std::auto_ptr<ResponseTimeCounter> 
ResponseTimeCounterFactoryImpl::createCounter(const std::string &name, bool want_real) {
    (void)want_real;
    return std::auto_ptr<ResponseTimeCounter>(new ResponseTimeCounterImpl(name));
}

static ComponentImplRegisterer<ResponseTimeCounterFactory> reg_(new ResponseTimeCounterFactoryImpl());
}

