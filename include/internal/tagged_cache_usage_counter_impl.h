#ifndef _XSCRIPT_INTERNAL_CACHE_USAGE_COUNTER_IMPL_H
#define _XSCRIPT_INTERNAL_CACHE_USAGE_COUNTER_IMPL_H

#include <string>
#include <map>
#include <set>

#include <boost/shared_ptr.hpp>

#include "xscript/refresher.h"
#include "xscript/tagged_cache_usage_counter.h"
#include "internal/counter_impl.h"

namespace xscript {

class Script;
class TaggedBlock;

/**
 * Counter for measure tagged cache usage statistic.
 */
class TaggedCacheUsageCounterImpl : virtual public TaggedCacheUsageCounter, virtual public CounterImpl {
public:
    TaggedCacheUsageCounterImpl(const std::string &name);
    virtual XmlNodeHelper createReport() const;

protected:
    struct RecordInfo {
            RecordInfo();
            RecordInfo(const std::string &key, const std::string &info);
            
            boost::uint64_t calls();
            double hitRatio();
            bool isGoodHitRatio();
            
            boost::uint64_t hits_;
            boost::uint64_t misses_;
            std::string key_;
            std::string info_;
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
        
        typedef std::set<RecordInfoPtr, RecordComparator>::iterator RecordIterator;
        typedef std::multiset<RecordInfoPtr, RecordHitComparator>::iterator RecordHitIterator;
        typedef std::multiset<RecordInfoPtr, RecordHitComparator>::const_iterator RecordHitConstIterator;
        
        void eraseRecord(RecordIterator it);
        
private:
    void refresh();
    
protected:    
    std::set<RecordInfoPtr, RecordComparator> records_;
    std::multiset<RecordInfoPtr, RecordHitComparator> records_by_ratio_;
    
private:
    std::auto_ptr<Refresher> refresher_;

};

class TaggedCacheScriptUsageCounterImpl : public TaggedCacheUsageCounterImpl {
public:
    TaggedCacheScriptUsageCounterImpl(const std::string &name);
    
    void fetchedHit(const Context *ctx, const Object *obj, const TagKey *key);
    void fetchedMiss(const Context *ctx, const Object *obj, const TagKey *key);
    
private:
    void fetched(const Context *ctx, const Script *script, const TagKey *key, bool is_hit);
};

class TaggedCacheBlockUsageCounterImpl : public TaggedCacheUsageCounterImpl {
public:
    TaggedCacheBlockUsageCounterImpl(const std::string &name);
    
    void fetchedHit(const Context *ctx, const Object *obj, const TagKey *key);
    void fetchedMiss(const Context *ctx, const Object *obj, const TagKey *key);
    
private:
    void fetched(const Context *ctx, const TaggedBlock *block, const TagKey *key, bool is_hit);
};

}

#endif
