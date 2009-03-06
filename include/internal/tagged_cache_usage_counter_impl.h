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

    void fetched(const TagKey *key, const TaggedBlock *block);

private:    
    struct RecordInfo {
        RecordInfo() : hit_ratio_(0.0), calls_(0) {}
        double hit_ratio_;
        boost::uint64_t calls_;
        std::string block_info_;
        std::set<std::string> owners_;
    };
    typedef boost::shared_ptr<RecordInfo> RecordInfoPtr;
    
    struct RecordComparator {
        bool operator() (RecordInfoPtr r1, RecordInfoPtr r2) const {
            return r1->hit_ratio_ > r2->hit_ratio_;
        }
    };
    
    std::map<std::string, RecordInfoPtr> records_;
    std::multiset<RecordInfoPtr, RecordComparator> records_by_ratio_;
    typedef std::multiset<RecordInfoPtr, RecordComparator>::iterator RecordIterator;
    typedef std::multiset<RecordInfoPtr, RecordComparator>::const_iterator RecordConstIterator;
};

}

#endif
