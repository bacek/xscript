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

TaggedCacheUsageCounterImpl::TaggedCacheUsageCounterImpl(const std::string& name)
        : CounterImpl(name) {
}

void
TaggedCacheUsageCounterImpl::fetched(const TagKey *key, const TaggedBlock *block) {
    boost::mutex::scoped_lock lock(mtx_);    
    const std::string& block_key = key->asString();    
    std::map<std::string, RecordInfoPtr>::iterator it = records_.find(block_key);
    if (it == records_.end()) {
        RecordInfoPtr record(new RecordInfo());
        record->hit_ratio_ = 0.0;
        record->calls_ = 1;
        record->block_info_ = block->name();
        record->owners_.insert(block->owner()->name());
        
        records_.insert(std::make_pair(block_key, record));
        records_by_ratio_.insert(record);
    }
    else {
        RecordInfoPtr record = it->second;
        records_.erase(it);

        std::pair<RecordIterator, RecordIterator> range = records_by_ratio_.equal_range(record);        
        for(RecordIterator rit = range.first; rit != range.second; ++rit) {
            if (rit->get() == record.get()) {
                records_by_ratio_.erase(rit);
                break;
            }
        }
        
        double t = 1.0/record->calls_;
        record->hit_ratio_ = (record->hit_ratio_ + t)/(t + 1.0);
        record->calls_++;
        record->owners_.insert(block->owner()->name());
        
        records_.insert(std::make_pair(block_key, record));
        records_by_ratio_.insert(record);
    } 
    
}

XmlNodeHelper
TaggedCacheUsageCounterImpl::createReport() const {
    XmlNodeHelper root(xmlNewNode(0, (const xmlChar*) name_.c_str()));
    
    boost::mutex::scoped_lock lock(mtx_);

    int count = 0;
    for(typename RecordIterator it = records_by_ratio_.begin();
        it != records_by_ratio_.end();
        ++it, ++count) {
        
        if (count == 5) {
            break;
        }
        
        xmlNodePtr line = xmlNewChild(root.get(), NULL, (const xmlChar*)"block", NULL);
        
        xmlSetProp(line, (const xmlChar*)"hit-ratio",
                (const xmlChar*) boost::lexical_cast<std::string>((*it)->hit_ratio_).c_str());
        
        xmlSetProp(line, (const xmlChar*)"calls",
                (const xmlChar*) boost::lexical_cast<std::string>((*it)->calls_).c_str());
        
        xmlSetProp(line, (const xmlChar*) "info", (const xmlChar*)((*it)->block_info_.c_str()));
        
        for(std::set<std::string>::iterator it_owner = (*it)->owners_.begin();
            it_owner != (*it)->owners_.end();
            ++it_owner) {
            xmlNewTextChild(line, NULL, (const xmlChar*)"owner", (const xmlChar*)it_owner->c_str());
        }
    }

    return root;
}

}
