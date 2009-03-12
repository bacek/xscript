#ifndef _XSCRIPT_INTERNAL_CACHE_USAGE_COUNTER_IMPL_H
#define _XSCRIPT_INTERNAL_CACHE_USAGE_COUNTER_IMPL_H

#include <string>
#include <map>
#include <set>

#include <boost/shared_ptr.hpp>

#include "xscript/tagged_cache_usage_counter.h"
#include "internal/counter_impl.h"

namespace xscript {

/**
 * Counter for measure tagged cache usage statistic.
 */
class TaggedCacheUsageCounterImpl : virtual public TaggedCacheUsageCounter, virtual private CounterImpl {
public:
    TaggedCacheUsageCounterImpl(const std::string &name);
    virtual XmlNodeHelper createReport() const;

    void fetchedHit(const Context *ctx, const TaggedBlock *block);
    void fetchedMiss(const Context *ctx, const TaggedBlock *block);

private:
    
    void fetched(const Context *ctx, const TaggedBlock *block, bool is_hit);
    
    struct RecordInfo {
        RecordInfo();
        RecordInfo(const std::string &block_info);
        
        boost::uint64_t calls();
        double hitRatio();
        bool isGoodHitRatio();
        
        boost::uint64_t hits_;
        boost::uint64_t misses_;
        std::string block_info_;
        time_t last_call_time_;
        std::map<std::string, boost::uint64_t> owners_;
    };
    typedef boost::shared_ptr<RecordInfo> RecordInfoPtr;
    
    struct RecordComparator {
        bool operator() (RecordInfoPtr r1, RecordInfoPtr r2) const;        
    };    
    struct RecordHitComparator {
        bool operator() (RecordInfoPtr r1, RecordInfoPtr r2) const;
    };
    
    std::set<RecordInfoPtr, RecordComparator> records_;
    std::multiset<RecordInfoPtr, RecordHitComparator> records_by_ratio_;
    typedef std::set<RecordInfoPtr, RecordComparator>::iterator RecordIterator;
    typedef std::multiset<RecordInfoPtr, RecordHitComparator>::iterator RecordHitIterator;
    typedef std::multiset<RecordInfoPtr, RecordHitComparator>::const_iterator RecordHitConstIterator;
};

}

#endif
