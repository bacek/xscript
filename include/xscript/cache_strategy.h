#ifndef _XSCRIPT_CACHE_STATEGY_H_
#define _XSCRIPT_CACHE_STATEGY_H_

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace xscript {

class Config;
class Context;

class SubCacheStrategy {
public:
    SubCacheStrategy();
    virtual ~SubCacheStrategy();
    
    virtual void initContext(Context *ctx);
    virtual std::string createKey(const Context *ctx);
    virtual bool noCache(const Context *ctx) const;
};

class SubCacheStrategyFactory {
public:
    SubCacheStrategyFactory();
    virtual ~SubCacheStrategyFactory();

    virtual std::auto_ptr<SubCacheStrategy> create(const Config *config, const std::string &path) = 0;
};

class CacheStrategy {
public:
    CacheStrategy();
    virtual ~CacheStrategy();
    
    void add(std::auto_ptr<SubCacheStrategy> substrategy);
    
    void initContext(Context *ctx);
    std::string createKey(const Context *ctx) const;
    bool noCache(const Context *ctx) const;
private:
    std::vector<boost::shared_ptr<SubCacheStrategy> > substrategies_;
};

} // namespace xscript

#endif // _XSCRIPT_CACHE_STATEGY_H_
