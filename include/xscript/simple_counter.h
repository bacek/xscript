#ifndef _XSCRIPT_SIMPLE_COUNTER_H_
#define _XSCRIPT_SIMPLE_COUNTER_H_

#include <string>
#include <memory>
#include <xscript/component.h>
#include <xscript/counter_base.h>

namespace xscript {

class SimpleCounter : public CounterBase {
public:
    virtual void inc() = 0;
    virtual void dec() = 0;

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
    friend class ComponentRegisterer<SimpleCounterFactory>;

    virtual std::auto_ptr<SimpleCounter> createCounter(const std::string& name) = 0;
};

} // namespace xscript

#endif // _XSCRIPT_SIMPLE_COUNTER_H_
