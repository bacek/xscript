#include "settings.h"

#include <boost/lexical_cast.hpp>
#include "internal/tagged_cache_usage_counter_impl.h"

#include "xscript/doc_cache_strategy.h"
#include "xscript/tagged_block.h"
#include "xscript/xml.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

TaggedCacheUsageCounterImpl::TaggedCacheUsageCounterImpl(const std::string &name)
        : CounterImpl(name) {
    boost::function<void()> func = boost::bind(&TaggedCacheUsageCounterImpl::refresh, this);
    refresher_.reset(new Refresher(func, TaggedCacheUsageCounterFactory::refreshTime()));
}

void
TaggedCacheUsageCounterImpl::fetched(const Context *ctx, const TaggedBlock *block, bool is_hit) {
    RecordInfoPtr record(new RecordInfo(block->info(ctx)));
    RecordIterator it = records_.find(record);
    if (it != records_.end()) {
        record = *it;
        eraseRecord(it);
    }

    record->last_call_time_ = time(NULL);
    
    if (is_hit) {
        ++record->hits_;
    }
    else {
        ++record->misses_;
    }
    
    const std::string& owner_name = block->owner()->name();
    std::map<std::string, boost::uint64_t>::iterator it_owner = record->owners_.find(owner_name);
    if (it_owner == record->owners_.end()) {
        record->owners_.insert(std::make_pair(owner_name, 1));
    }
    else {
        ++(it_owner->second);
    }
    
    records_.insert(record);
    records_by_ratio_.insert(record);
}

void
TaggedCacheUsageCounterImpl::fetchedHit(const Context *ctx, const TaggedBlock *block) {
    boost::mutex::scoped_lock lock(mtx_);
    fetched(ctx, block, true);
}

void
TaggedCacheUsageCounterImpl::fetchedMiss(const Context *ctx, const TaggedBlock *block) {
    boost::mutex::scoped_lock lock(mtx_);
    fetched(ctx, block, false);
}

XmlNodeHelper
TaggedCacheUsageCounterImpl::createReport() const {
    XmlNodeHelper root(xmlNewNode(0, (const xmlChar*) name_.c_str()));
    
    boost::mutex::scoped_lock lock(mtx_);

    unsigned int count = 0;
    for(RecordHitConstIterator it = records_by_ratio_.begin();
        it != records_by_ratio_.end();
        ++it) {
        
        if (count++ == TaggedCacheUsageCounterFactory::outputSize() ||
            (*it)->hitRatio() > TaggedCacheUsageCounterFactory::hitRatioLevel()) {
            break;
        }
        
        xmlNodePtr line = xmlNewChild(root.get(), NULL, (const xmlChar*)"block", NULL);
        
        xmlSetProp(line, (const xmlChar*)"hit-ratio",
                (const xmlChar*) boost::lexical_cast<std::string>((*it)->hitRatio()).c_str());
        
        xmlSetProp(line, (const xmlChar*)"calls",
                (const xmlChar*) boost::lexical_cast<std::string>((*it)->calls()).c_str());
        
        xmlSetProp(line, (const xmlChar*) "info", (const xmlChar*)((*it)->block_info_.c_str()));
        
        std::multimap<boost::uint64_t, const char*, std::greater<boost::uint64_t> > owners;        
        for(std::map<std::string, boost::uint64_t>::iterator it_owner = (*it)->owners_.begin();
            it_owner != (*it)->owners_.end();
            ++it_owner) {            
            owners.insert(std::make_pair(it_owner->second, it_owner->first.c_str()));
        }
        
        for(std::multimap<boost::uint64_t, const char*, std::greater<boost::uint64_t> >::iterator it_owner = owners.begin();
            it_owner != owners.end();
            ++it_owner) {           
            xmlNodePtr owner_line = xmlNewChild(line, NULL, (const xmlChar*)"owner",
                    (const xmlChar*)it_owner->second);
            
            xmlSetProp(owner_line, (const xmlChar*)"calls",
                    (const xmlChar*) boost::lexical_cast<std::string>(it_owner->first).c_str());
        }
    }

    return root;
}

void
TaggedCacheUsageCounterImpl::refresh() {
    boost::mutex::scoped_lock lock(mtx_);
    
    time_t now = time(NULL);
    for(RecordIterator it = records_.begin(); it != records_.end(); ++it) {        
        if (now - (*it)->last_call_time_ <= (time_t)TaggedCacheUsageCounterFactory::maxIdleTime()) {
            continue;
        }
        eraseRecord(it);
    }
}

void
TaggedCacheUsageCounterImpl::eraseRecord(RecordIterator it) {
    std::pair<RecordHitIterator, RecordHitIterator> range = records_by_ratio_.equal_range(*it);        
    for(RecordHitIterator rit = range.first; rit != range.second; ++rit) {
        if (rit->get() == it->get()) {
            records_by_ratio_.erase(rit);
            break;
        }
    }
    
    records_.erase(it);
}

TaggedCacheUsageCounterImpl::RecordInfo::RecordInfo() :
    hits_(0), misses_(0), last_call_time_(0)
{}

TaggedCacheUsageCounterImpl::RecordInfo::RecordInfo(const std::string &block_info) :
    hits_(0), misses_(0), block_info_(block_info), last_call_time_(0)
{}

boost::uint64_t
TaggedCacheUsageCounterImpl::RecordInfo::calls() {
    return hits_ + misses_;
}

double
TaggedCacheUsageCounterImpl::RecordInfo::hitRatio() {
    return (double)hits_/calls();
}

bool
TaggedCacheUsageCounterImpl::RecordInfo::isGoodHitRatio() {
    return hitRatio() > TaggedCacheUsageCounterFactory::hitRatioLevel();
}


bool
TaggedCacheUsageCounterImpl::RecordComparator::operator() (RecordInfoPtr r1, RecordInfoPtr r2) const {
    return r1->block_info_ < r2->block_info_;
}        

bool
TaggedCacheUsageCounterImpl::RecordHitComparator::operator() (RecordInfoPtr r1, RecordInfoPtr r2) const {
    if (r1->hitRatio() > TaggedCacheUsageCounterFactory::hitRatioLevel()) {
        return false;
    }            
    if (r2->hitRatio() > TaggedCacheUsageCounterFactory::hitRatioLevel()) {
        return true;
    }            
    return r1->calls() > r2->calls();
}


}
