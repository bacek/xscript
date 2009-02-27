#ifndef _XSCRIPT_SIMPLE_COUNTER_H_
#define _XSCRIPT_SIMPLE_COUNTER_H_

#include <string>
#include <memory>
#include <boost/cstdint.hpp>
#include <xscript/component.h>
#include <xscript/counter_base.h>

namespace xscript {

class SimpleCounter : public CounterBase {
public:
    virtual void inc() = 0;
    virtual void dec() = 0;
    virtual void max(boost::uint64_t val) = 0;

    class ScopedCount {
    public:
        ScopedCount(SimpleCounter * counter)
                : counter_(counter) {
            counter_->inc();
        }
        ~ScopedCount() {
            counter_->dec();
        }
    private:
        SimpleCounter * counter_;

    };

};

class SimpleCounterFactory : public Component<SimpleCounterFactory> {
public:
    SimpleCounterFactory();
    ~SimpleCounterFactory();

    virtual void init(const Config *config);  
    virtual std::auto_ptr<SimpleCounter> createCounter(const std::string& name, bool want_real = false);
};

} // namespace xscript

#endif // _XSCRIPT_SIMPLE_COUNTER_H_
