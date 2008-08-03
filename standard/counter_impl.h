#ifndef _XSCRIPT_STANDART_COUNTER_IMPL_H_
#define _XSCRIPT_STANDART_COUNTER_IMPL_H_

#include <boost/thread/mutex.hpp>
#include <xscript/counter_base.h>

namespace xscript 
{
    /**
     * Holder for name and mutex common for all counters.
     */
    class CounterImpl : public CounterBase {
    public:
        CounterImpl(const std::string name) 
            : name_(name)
        {
        }

    protected:
        std::string     name_;
		boost::mutex    mtx_;
    };

} // namespace xscript

#endif
