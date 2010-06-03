#include "settings.h"

#include <string.h>
#include <stdexcept>

#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include "xscript/algorithm.h"
#include "xscript/block.h"
#include "xscript/cached_object.h"
#include "xscript/cache_strategy_collector.h"
#include "xscript/doc_cache_strategy.h"
#include "xscript/logger.h"
#include "xscript/operation_mode.h"
#include "xscript/param.h"
#include "xscript/range.h"
#include "xscript/string_utils.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

static int default_cache_strategy = CachedObject::UNKNOWN;
const time_t CachedObject::CACHE_TIME_UNDEFINED = std::numeric_limits<time_t>::max();

class CachedObject::ObjectData {
public:
    ObjectData();
    ~ObjectData();
    
    int strategy_;
    boost::shared_ptr<CacheStrategy> cache_strategy_;
    time_t cache_time_;
};

CachedObject::ObjectData::ObjectData() :
    strategy_(default_cache_strategy), cache_time_(CACHE_TIME_UNDEFINED)
{}

CachedObject::ObjectData::~ObjectData()
{}

CachedObject::CachedObject() : data_(new ObjectData())
{}

CachedObject::~CachedObject() {
}

time_t
CachedObject::cacheTime() const {
    return data_->cache_time_;
}

void
CachedObject::cacheTime(time_t cache_time) {
    data_->cache_time_ = cache_time;
}

bool
CachedObject::cacheTimeUndefined() const {
    return data_->cache_time_ == CACHE_TIME_UNDEFINED;
}

bool
CachedObject::checkProperty(const char *name, const char *value) {
    if (strncasecmp(name, "cache-strategy", sizeof("cache-strategy")) != 0) {
        return false;
    }
    
    Range strategy = trim(createRange(value));
    if (strategy.empty()) {
        throw std::runtime_error("empty cache strategy is not allowed");
    }
    
    bool cache_type = false;
    do {
        Range key;
        split(strategy, ' ', key, strategy);
        key = trim(key);
        if (key.empty()) {
            continue;
        }
        
        if (key.size() == sizeof("distributed") - 1 &&
            strncasecmp(key.begin(), "distributed", sizeof("distributed") - 1) == 0) {
            if (!cache_type) {
                cache_type = true;
                data_->strategy_ = UNKNOWN;
            }
            data_->strategy_ |= DISTRIBUTED;
        }
        else if (key.size() == sizeof("local") - 1 &&
                 strncasecmp(key.begin(), "local", sizeof("local") - 1) == 0) {
            if (!cache_type) {
                cache_type = true;
                data_->strategy_ = UNKNOWN;
            }
            data_->strategy_ |= LOCAL;
        }
        else {
            if (NULL != data_->cache_strategy_.get()) {
                throw std::runtime_error("only one cache strategy allowed");
            }
                       
            Range strategy_name, cache_time;
            split(key, ':', strategy_name, cache_time);
            strategy_name = trim(strategy_name);
            cache_time = trim(cache_time);
                      
            std::string str_name(strategy_name.begin(), strategy_name.end());
            
            boost::shared_ptr<CacheStrategy> cache_strategy =
                CacheStrategyCollector::instance()->pageStrategy(str_name);
            
            if (NULL == cache_strategy.get()) {
                OperationMode::instance()->processError("unknown cache strategy: " + str_name);
            }
            else {
                data_->cache_strategy_ = cache_strategy;
            }
            
            if (cache_time.empty()) {
                throw std::runtime_error("Cache time for strategy is not specified");
            }
            
            std::string cache_time_str(cache_time.begin(), cache_time.end());
            try {
                cacheTime(boost::lexical_cast<time_t>(cache_time_str));
            }
            catch(const boost::bad_lexical_cast &e) {
                throw std::runtime_error(
                    std::string("cannot parse cache time value: ") + cache_time_str);
            }
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

CacheStrategy*
CachedObject::cacheStrategy() const {
    return data_->cache_strategy_.get();
}

std::string
CachedObject::fileModifiedKey(const std::string &filename) {
    if (filename.empty()) {
        return StringUtils::EMPTY_STRING;
    }
    
    namespace fs = boost::filesystem;
    fs::path path(filename);
    if (fs::exists(path) && !fs::is_directory(path)) {
        return boost::lexical_cast<std::string>(fs::last_write_time(path));
    }
    
    throw std::runtime_error("Cannot stat filename " + filename);
}

std::string
CachedObject::modifiedKey(const Xml::TimeMapType &modified_info) {
    std::string key;
    for(Xml::TimeMapType::const_iterator it = modified_info.begin();
        it != modified_info.end();
        ++it) {
        if (!key.empty()) {
            key.push_back('|');
        }
        key.append(boost::lexical_cast<std::string>(it->second));
    }
    return key;
}

std::string
CachedObject::blocksModifiedKey(const std::vector<Block*> &blocks) {
    std::string key;
    for(std::vector<Block*>::const_iterator it = blocks.begin();
        it != blocks.end();
        ++it) {
        if (!key.empty()) {
            key.push_back('|');
        }
        key.append(fileModifiedKey((*it)->xsltName()));
    }
    return key;
}

std::string
CachedObject::paramsKey(const std::vector<Param*> &params, const Context *ctx) {
    std::string key;
    for(std::vector<Param*>::const_iterator it = params.begin();
        it != params.end();
        ++it) {
        if (!key.empty()) {
            key.push_back(':');
        }
        key.append((*it)->asString(ctx));
    }
    return key;
}

std::string
CachedObject::paramsIdKey(const std::vector<Param*> &params, const Context *ctx) {
    std::string key;
    for(std::vector<Param*>::const_iterator it = params.begin();
        it != params.end();
        ++it) {
        if (!key.empty()) {
            key.push_back(':');
        }
        key.append((*it)->id());
        key.push_back(':');
        key.append((*it)->asString(ctx));
    }
    return key;
}

} // namespace xscript
