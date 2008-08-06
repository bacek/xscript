#ifndef _XSCRIPT_DETAILS_DUMMY_SIMPLE_COUNTER_H_
#define _XSCRIPT_DETAILS_DUMMY_SIMPLE_COUNTER_H_

#include "xscript/simple_counter.h"

namespace xscript {
class DummySimpleCounter : public SimpleCounter {
public:
    DummySimpleCounter();
    ~DummySimpleCounter();

    virtual void inc();
    virtual void dec();

    virtual XmlNodeHelper createReport() const;
};

class DummySimpleCounterFactory : public SimpleCounterFactory {
public:
    DummySimpleCounterFactory();
    ~DummySimpleCounterFactory();

    virtual void init(const Config *config);

    std::auto_ptr<SimpleCounter> createCounter(const std::string& name);
};
}

#endif
