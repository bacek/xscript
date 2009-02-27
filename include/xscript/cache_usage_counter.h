#ifndef _XSCRIPT_CACHE_USAGE_COUNTER_H_
#define _XSCRIPT_CACHE_USAGE_COUNTER_H_

#include <string>
#include <boost/cstdint.hpp>

#include <xscript/counter_base.h>
#include <xscript/component.h>

namespace xscript {

    /**
    * Counter for measure cache statistic.
    * Trivial set of (loaded, stored, removed) counters.
    */
    class CacheUsageCounter : public CounterBase {
    public:
        virtual void fetched(const std::string& name) = 0;
        virtual void stored(const std::string& name) = 0;
        virtual void removed(const std::string& name) = 0;
        virtual void reset() = 0;
        virtual void max(boost::uint64_t val) = 0;
    };


    class CacheUsageCounterFactory : public Component<CacheUsageCounterFactory> {
    public:
        CacheUsageCounterFactory();
        ~CacheUsageCounterFactory();

        virtual void init(const Config *config);      
        virtual std::auto_ptr<CacheUsageCounter> createCounter(const std::string& name, bool want_real = false);
    };

} // namespace xscript

#endif // _XSCRIPT_CACHE_USAGE_COUNTER_H_
