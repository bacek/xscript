#ifndef _XSCRIPT_INTERNAL_CACHE_STRATEGY_COLLECTOR_H_
#define _XSCRIPT_INTERNAL_CACHE_STRATEGY_COLLECTOR_H_

#include <map>
#include <string>
#include <vector>

#include <boost/thread/mutex.hpp>
#include <boost/shared_ptr.hpp>

#include "xscript/cache_strategy.h"

namespace xscript {

class Config;
class DocCacheStrategy;

class CacheStrategyCollector {
public:
    CacheStrategyCollector();
    virtual ~CacheStrategyCollector();
    
    static CacheStrategyCollector* instance();
    
    void init(const Config *config);
    
    void addStrategy(DocCacheStrategy *strategy, const std::string &name);
    
    void addPageStrategyHandler(const std::string &tag,
            const boost::shared_ptr<SubCacheStrategyFactory> &handler);
    
    void addBlockStrategyHandler(const std::string &tag,
            const boost::shared_ptr<SubCacheStrategyFactory> &handler);
    
    boost::shared_ptr<CacheStrategy> blockStrategy(const std::string &name) const;
    boost::shared_ptr<CacheStrategy> pageStrategy(const std::string &name) const;

private:
    typedef std::map<std::string, boost::shared_ptr<CacheStrategy> > StrategyMapType;
    typedef std::map<std::string, boost::shared_ptr<SubCacheStrategyFactory> > HandlerMapType;

    void parsePageCacheStrategies(const Config *config, StrategyMapType &strategies);
    void swapPageStrategies(StrategyMapType &strategies);

private:
    std::vector<std::pair<DocCacheStrategy*, std::string> > strategies_;
    
    HandlerMapType block_strategy_handlers_;
    HandlerMapType page_strategy_handlers_;

    StrategyMapType block_strategies_;
    StrategyMapType page_strategies_;
    mutable boost::mutex page_strategies_mutex_;

    class CacheStrategyBlock;
    friend class CacheStrategyBlock;
};

} // namespace xscript

#endif // _XSCRIPT_INTERNAL_CACHE_STRATEGY_COLLECTOR_H_
