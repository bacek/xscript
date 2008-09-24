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
    virtual void max(uint64_t val);

    virtual XmlNodeHelper createReport() const;
};

}

#endif
