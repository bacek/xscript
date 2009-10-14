#ifndef _XSCRIPT_DETAILS_DUMMY_RESPONSE_TIME_COUNTER_H
#define _XSCRIPT_DETAILS_DUMMY_RESPONSE_TIME_COUNTER_H

#include "xscript/response_time_counter.h"

namespace xscript {
/**
 * Do nothing counter
 */
class DummyResponseTimeCounter : public ResponseTimeCounter {
public:
    virtual void add(const Response *resp, boost::uint64_t value);
    virtual void add(const Context *ctx, boost::uint64_t value);
    virtual XmlNodeHelper createReport() const;
    virtual void reset();
};

}

#endif



