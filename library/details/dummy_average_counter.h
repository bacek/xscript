#ifndef _XSCRIPT_DETAILS_DUMMY_AVERAGE_COUNTER_H
#define _XSCRIPT_DETAILS_DUMMY_AVERAGE_COUNTER_H

#include "xscript/average_counter.h"

namespace xscript {


/**
 * Do nothing counter
 */
class DummyAverageCounter : public AverageCounter {
public:
    DummyAverageCounter();
    ~DummyAverageCounter();

    /**
     * Add single measure.
     */
    virtual void add(uint64_t value);

    /**
     * Remove single measure.
     */
    virtual void remove(uint64_t value);
    
    virtual boost::uint64_t count() const;

    virtual XmlNodeHelper createReport() const;
};


}

#endif


