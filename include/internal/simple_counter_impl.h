#ifndef _XSCRIPT_INTERNAL_SIMPLE_COUNTER_IMPL_H_
#define _XSCRIPT_INTERNAL_SIMPLE_COUNTER_IMPL_H_

#include <boost/thread/mutex.hpp>
#include <boost/cstdint.hpp>
#include "xscript/simple_counter.h"
#include "internal/counter_impl.h"

namespace xscript {

class SimpleCounterImpl : public SimpleCounter, private CounterImpl {
public:
    SimpleCounterImpl(const std::string& name);

    virtual XmlNodeHelper createReport() const;

    virtual void inc();
    virtual void dec();
    virtual void max(uint64_t val);

private:
    uint64_t count_;
    uint64_t peak_;
    uint64_t max_;
};

}

#endif
