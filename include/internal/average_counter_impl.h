#ifndef _XSCRIPT_INTERNAL_AVERAGE_COUNTER_IMPL_H
#define _XSCRIPT_INTERNAL_AVERAGE_COUNTER_IMPL_H

#include <boost/cstdint.hpp>
#include "xscript/average_counter.h"
#include "internal/counter_impl.h"

namespace xscript {
/**
 * Time statistic gatherer. min/max/avg time for call.
 */
class AverageCounterImpl : public AverageCounter, private CounterImpl {
public:
    AverageCounterImpl(const std::string& name);
    ~AverageCounterImpl();

    /**
     * Add single measure.
     */
    virtual void add(boost::uint64_t value);

    /**
     * Remove single measure.
     */
    virtual void remove(boost::uint64_t value);

    virtual boost::uint64_t count() const;
    
    virtual XmlNodeHelper createReport() const;

protected:
    boost::uint64_t count_;
    boost::uint64_t total_;
    boost::uint64_t max_;
    boost::uint64_t min_;
};
}

#endif
