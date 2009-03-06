#ifndef _XSCRIPT_TAGGED_CACHE_USAGE_COUNTER_H_
#define _XSCRIPT_TAGGED_CACHE_USAGE_COUNTER_H_

#include <string>
#include <boost/cstdint.hpp>

#include <xscript/counter_base.h>
#include <xscript/component.h>

namespace xscript {

class TaggedBlock;
class TagKey;

class TaggedCacheUsageCounter : public CounterBase {
public:
    virtual void fetched(const TagKey *key, const TaggedBlock *block) = 0;
};


class TaggedCacheUsageCounterFactory : public Component<TaggedCacheUsageCounterFactory> {
public:
    TaggedCacheUsageCounterFactory();
    ~TaggedCacheUsageCounterFactory();

    virtual void init(const Config *config);      
    virtual std::auto_ptr<TaggedCacheUsageCounter> createCounter(const std::string& name, bool want_real = false);
};

} // namespace xscript

#endif // _XSCRIPT_TAGGED_CACHE_USAGE_COUNTER_H_
