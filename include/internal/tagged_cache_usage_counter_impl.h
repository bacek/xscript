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
    TaggedCacheUsageCounterImpl(const std::string& name);
    virtual XmlNodeHelper createReport() const;

    void fetchedHit(const TagKey *key, const TaggedBlock *block);
    void fetchedMiss(const TagKey *key, const TaggedBlock *block);

private:
    
    void fetched(const TagKey *key, const TaggedBlock *block, bool is_hit);
    
    struct RecordInfo {
        RecordInfo() : hits_(0), misses_(0) {}
        boost::uint64_t calls() {return hits_ + misses_;}
        double hitRatio() {return (double)hits_/calls();}
        bool isGoodHitRatio() {
            return hitRatio() > TaggedCacheUsageCounterFactory::hitRatioLevel();
        }
        boost::uint64_t hits_;
        boost::uint64_t misses_;
        std::string block_info_;
        std::set<std::string> owners_;
    };
    typedef boost::shared_ptr<RecordInfo> RecordInfoPtr;
    
    struct RecordComparator {
        bool operator() (RecordInfoPtr r1, RecordInfoPtr r2) const {
            if (r1->hitRatio() > TaggedCacheUsageCounterFactory::hitRatioLevel()) {
                return false;
            }
            
            if (r2->hitRatio() > TaggedCacheUsageCounterFactory::hitRatioLevel()) {
                return true;
            }
            
            return r1->hits_ + r1->misses_ > r2->hits_ + r2->misses_;
        }
    };
    
    std::map<std::string, RecordInfoPtr> records_;
    std::multiset<RecordInfoPtr, RecordComparator> records_by_ratio_;
    typedef std::multiset<RecordInfoPtr, RecordComparator>::iterator RecordIterator;
    typedef std::multiset<RecordInfoPtr, RecordComparator>::const_iterator RecordConstIterator;
};

}

#endif
