#ifndef _XSCRIPT_TAGGED_CACHE_USAGE_COUNTER_H_
#define _XSCRIPT_TAGGED_CACHE_USAGE_COUNTER_H_

#include <string>
#include <boost/cstdint.hpp>

#include <xscript/counter_base.h>
#include <xscript/component.h>

namespace xscript {

class CachedObject;
class Context;
class TagKey;

class TaggedCacheUsageCounter : public CounterBase {
public:
    virtual void fetchedHit(const Context *ctx, const CachedObject *obj) = 0;
    virtual void fetchedMiss(const Context *ctx, const CachedObject *obj) = 0;
};

class TaggedCacheUsageCounterFactory : public Component<TaggedCacheUsageCounterFactory> {
public:
    TaggedCacheUsageCounterFactory();
    ~TaggedCacheUsageCounterFactory();

    virtual void init(const Config *config);      
    virtual std::auto_ptr<TaggedCacheUsageCounter> createBlockCounter(const std::string& name, bool want_real = false);
    virtual std::auto_ptr<TaggedCacheUsageCounter> createScriptCounter(const std::string& name, bool want_real = false);
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
