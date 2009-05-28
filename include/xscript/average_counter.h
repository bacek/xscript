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
    
    virtual boost::uint64_t count() const = 0;
};

class AverageCounterFactory : public Component<AverageCounterFactory> {
public:
    AverageCounterFactory();
    ~AverageCounterFactory();

    virtual void init(const Config *config);
    virtual std::auto_ptr<AverageCounter> createCounter(const std::string& name, bool want_real = false);
};

} // namespace xscript

#endif // _XSCRIPT_AVERAGE_COUNTER_H_
