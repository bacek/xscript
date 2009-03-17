#ifndef _XSCRIPT_TAGGED_CACHE_USAGE_COUNTER_H_
#define _XSCRIPT_TAGGED_CACHE_USAGE_COUNTER_H_

#include <string>
#include <boost/cstdint.hpp>

#include <xscript/counter_base.h>
#include <xscript/component.h>

namespace xscript {

class Context;
class TaggedBlock;

class TaggedCacheUsageCounter : public CounterBase {
public:
    virtual void fetchedHit(const Context *ctx, const TaggedBlock *block) = 0;
    virtual void fetchedMiss(const Context *ctx, const TaggedBlock *block) = 0;
};

class TaggedCacheUsageCounterFactory : public Component<TaggedCacheUsageCounterFactory> {
public:
    TaggedCacheUsageCounterFactory();
    ~TaggedCacheUsageCounterFactory();

    virtual void init(const Config *config);      
    virtual std::auto_ptr<TaggedCacheUsageCounter> createCounter(const std::string& name, bool want_real = false);
    static unsigned int outputSize();
    static double hitRatioLevel();
    static boost::uint32_t refreshTime();
    static boost::uint32_t maxIdleTime();

private:
    static unsigned int output_size_;
    static double hit_ratio_level_;
    static boost::uint32_t refresh_time_;
    static boost::uint32_t max_idle_time_;
};

} // namespace xscript

#endif // _XSCRIPT_TAGGED_CACHE_USAGE_COUNTER_H_
