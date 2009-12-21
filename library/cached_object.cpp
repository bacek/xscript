#include "settings.h"

#include <string.h>
#include <stdexcept>

#include "xscript/algorithm.h"
#include "xscript/cached_object.h"
#include "xscript/range.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

static int default_cache_strategy = CachedObject::UNKNOWN;

class CachedObject::ObjectData {
public:
    ObjectData();
    ~ObjectData();
    
    int strategy_;
};

CachedObject::ObjectData::ObjectData() : strategy_(default_cache_strategy)
{}

CachedObject::ObjectData::~ObjectData()
{}

CachedObject::CachedObject() : data_(new ObjectData())
{}

CachedObject::~CachedObject() {
    delete data_;
}

bool
CachedObject::checkProperty(const char *name, const char *value) {
    if (strncasecmp(name, "cache-strategy", sizeof("cache-strategy")) != 0) {
        return false;
    }
    
    data_->strategy_ = UNKNOWN;
    Range strategy = trim(createRange(value));
    if (strategy.empty()) {
        throw std::runtime_error("empty cache strategy is not allowed");
    }
    do {
        Range key;
        split(strategy, ' ', key, strategy);
        key = trim(key);
        if (key.empty()) {
            continue;
        }
        
        if (key.size() == sizeof("distributed") - 1 &&
            strncasecmp(key.begin(), "distributed", sizeof("distributed") - 1) == 0) {
            data_->strategy_ |= DISTRIBUTED;
        }
        else if (key.size() == sizeof("local") - 1 &&
                 strncasecmp(key.begin(), "local", sizeof("local") - 1) == 0) {
            data_->strategy_ |= LOCAL;
        }
        else {
            throw std::runtime_error("incorrect cache-strategy value: " +
                    std::string(key.begin(), key.end()));
        }
    }
    while(!strategy.empty());

    return true;
}

bool
CachedObject::allowDistributed() const {
    return checkStrategy(DISTRIBUTED);
}

bool
CachedObject::checkStrategy(Strategy strategy) const {
    return data_->strategy_ & strategy;
}

void
CachedObject::addDefaultStrategy(Strategy strategy) {
    default_cache_strategy |= strategy;
}

void
CachedObject::clearDefaultStrategy(Strategy strategy) {
    default_cache_strategy &= ~strategy;
}


} // namespace xscript
