#ifndef _XSCRIPT_RESPONSE_TIME_COUNTER_H_
#define _XSCRIPT_RESPONSE_TIME_COUNTER_H_

#include <string>

#include <boost/cstdint.hpp>

#include <xscript/counter_base.h>
#include <xscript/component.h>

namespace xscript {

class Context;
class Response;

class ResponseTimeCounter : public CounterBase {
public:
    virtual void add(const Response *resp, boost::uint64_t value) = 0;
    virtual void add(const Context *ctx, boost::uint64_t value) = 0;
    virtual void reset() = 0;
};

class ResponseTimeCounterFactory : public Component<ResponseTimeCounterFactory> {
public:
    ResponseTimeCounterFactory();
    ~ResponseTimeCounterFactory();

    virtual void init(const Config *config);      
    virtual std::auto_ptr<ResponseTimeCounter> createCounter(const std::string& name, bool want_real = false);
};

} // namespace xscript

#endif // _XSCRIPT_RESPONSE_TIME_COUNTER_H_
