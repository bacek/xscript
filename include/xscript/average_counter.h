#ifndef _XSCRIPT_AVERAGE_COUNTER_H_
#define _XSCRIPT_AVERAGE_COUNTER_H_

#include <string>
#include <memory>
#include <boost/cstdint.hpp>

#include <xscript/component.h>
#include <xscript/counter_base.h>

namespace xscript {

/**
 * Time statistic gatherer. min/max/avg time for something.
 */
class AverageCounter : public CounterBase {

public:
    /**
     * Add single measure.
     */
    virtual void add(boost::uint64_t value) = 0;

    /**
     * Remove single measure.
     */
    virtual void remove(boost::uint64_t value) = 0;
};

class AverageCounterFactory : public Component<AverageCounterFactory> {
public:

    friend class ComponentRegisterer<AverageCounterFactory>;

    virtual std::auto_ptr<AverageCounter> createCounter(const std::string& name) = 0;
};

} // namespace xscript

#endif // _XSCRIPT_AVERAGE_COUNTER_H_
