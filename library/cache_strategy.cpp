#include "settings.h"

#include <set>
#include <string>
#include <stdexcept>

#include <boost/tokenizer.hpp>

#include "xscript/cache_strategy.h"
#include "xscript/config.h"
#include "xscript/context.h"
#include "xscript/doc_cache_strategy.h"
#include "xscript/policy.h"
#include "xscript/string_utils.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

SubCacheStrategy::SubCacheStrategy() {
}

SubCacheStrategy::~SubCacheStrategy() {
}

void
SubCacheStrategy::initContext(Context *ctx) {
    (void)ctx;
}

std::string
SubCacheStrategy::createKey(const Context *ctx) {
    (void)ctx;
    return StringUtils::EMPTY_STRING;
}

bool
SubCacheStrategy::noCache(const Context *ctx) const {
    (void)ctx;
    return false;
}

SubCacheStrategyFactory::SubCacheStrategyFactory()
{}

SubCacheStrategyFactory::~SubCacheStrategyFactory()
{}

CacheStrategy::CacheStrategy()
{
}

CacheStrategy::~CacheStrategy() {
}

void
CacheStrategy::add(std::auto_ptr<SubCacheStrategy> substrategy) {
    substrategies_.push_back(boost::shared_ptr<SubCacheStrategy>(substrategy.release()));
}

void
CacheStrategy::initContext(Context *ctx) {
    for(std::vector<boost::shared_ptr<SubCacheStrategy> >::const_iterator it = substrategies_.begin();
        it != substrategies_.end();
        ++it) {
        if ((*it)->noCache(ctx)) {
            ctx->rootContext()->setNoCache();
        }
        (*it)->initContext(ctx);
    }
}

std::string
CacheStrategy::createKey(const Context *ctx) const {
    std::string result;
    bool is_first = true;
    for(std::vector<boost::shared_ptr<SubCacheStrategy> >::const_iterator it = substrategies_.begin();
        it != substrategies_.end();
        ++it) {
        if (is_first) {
            is_first = false;
        }
        else {
            result.push_back('|');
        }
        result.append((*it)->createKey(ctx));
    }
    return result;
}

bool
CacheStrategy::noCache(const Context *ctx) const {
    for(std::vector<boost::shared_ptr<SubCacheStrategy> >::const_iterator it = substrategies_.begin();
        it != substrategies_.end();
        ++it) {
        if ((*it)->noCache(ctx)) {
            return true;
        }
    }
    return false;
}

class QuerySubCacheStrategy : public SubCacheStrategy {
public:
    QuerySubCacheStrategy();
    virtual std::string createKey(const Context *ctx);
    
    friend class QuerySubCacheStrategyFactory;
private:
    bool cacheAll() const;
    
private:
    std::set<std::string> cache_args_;
};

class QuerySubCacheStrategyFactory : public SubCacheStrategyFactory {
    std::auto_ptr<SubCacheStrategy> create(const Config *config, const std::string &path) {
        
        QuerySubCacheStrategy* query_strategy = new QuerySubCacheStrategy();
        std::auto_ptr<SubCacheStrategy> strategy(query_strategy);
        std::string value;
        try {
            value = config->as<std::string>(path);
        }
        catch(const std::exception &e) {
            return std::auto_ptr<SubCacheStrategy>();
        }

        typedef boost::char_separator<char> Separator;
        typedef boost::tokenizer<Separator> Tokenizer;
        Tokenizer tok(value, Separator(", "));
        for (Tokenizer::iterator it = tok.begin(), it_end = tok.end(); it != it_end; ++it) {
            query_strategy->cache_args_.insert(*it);
        }
        
        return strategy;
    }
};

QuerySubCacheStrategy::QuerySubCacheStrategy()
{}

bool
QuerySubCacheStrategy::cacheAll() const {
    return cache_args_.empty();
}

std::string
QuerySubCacheStrategy::createKey(const Context *ctx) {
    
    if (cacheAll()) {
        const std::string& query = ctx->request()->getQueryString();
        if (!query.empty()) {
            return "?" + query;
        }
        return StringUtils::EMPTY_STRING;
    }
    
    Request *request = ctx->request();
    std::vector<std::string> names;
    request->argNames(names);
    
    std::string key;
    bool is_first = true;
    for(std::vector<std::string>::const_iterator i = names.begin(), end = names.end();
        i != end;
        ++i) {
        std::string name = *i;
        std::vector<std::string> values;
        if (name.empty() || cache_args_.end() == cache_args_.find(name)) {            
            continue;
        }
        request->getArg(name, values);
        for(std::vector<std::string>::const_iterator it = values.begin(), end = values.end();
            it != end;
            ++it) {
            if (is_first) {
                is_first = false;
            }
            else {
                key.push_back('&');
            }
            key.append(name);
            key.push_back('=');
            key.append(*it);
        }
    }
    
    if (key.empty()) {
        return StringUtils::EMPTY_STRING;
    }
    
    return "?" + key;
}

class CookieSubCacheStrategy : public SubCacheStrategy {
public:
    virtual std::string createKey(const Context *ctx);
    
    friend class CookieSubCacheStrategyFactory;
private:
    std::set<std::string> cache_cookies_;
};

class CookieSubCacheStrategyFactory : public SubCacheStrategyFactory {
    std::auto_ptr<SubCacheStrategy> create(const Config *config, const std::string &path) {
               
        std::string value;
        try {
            value = config->as<std::string>(path);
        }
        catch(const std::exception &e) {
            return std::auto_ptr<SubCacheStrategy>();
        }
        
        CookieSubCacheStrategy* cookie_strategy = new CookieSubCacheStrategy();
        std::auto_ptr<SubCacheStrategy> strategy(cookie_strategy);
        
        typedef boost::char_separator<char> Separator;
        typedef boost::tokenizer<Separator> Tokenizer;
        Tokenizer tok(value, Separator(", "));
        for (Tokenizer::iterator it = tok.begin(), it_end = tok.end(); it != it_end; ++it) {
            if (!Policy::allowCachingInputCookie(it->c_str())) {
                std::stringstream ss;
                ss << "Cookie " << *it << " is not allowed in cookie strategy";
                throw std::runtime_error(ss.str());
            }
            cookie_strategy->cache_cookies_.insert(*it);
        }
        
        return strategy;
    }
};

std::string
CookieSubCacheStrategy::createKey(const Context *ctx) {
    std::string key;
    bool is_first = true;
    for(std::set<std::string>::iterator it = cache_cookies_.begin();
        it != cache_cookies_.end();
        ++it) {
        
        std::string cookie = Policy::getCacheCookie(ctx, *it);
        if (cookie.empty()) {
            continue;
        }

        if (is_first) {
            is_first = false;
        }
        else {
            key.push_back('|');
        }
        key.append(*it);
        key.push_back(':');
        key.append(cookie);
    }
    return key; 
}


class RandomSubCacheStrategy : public SubCacheStrategy {
public:
    RandomSubCacheStrategy(boost::int32_t max_value);
    virtual std::string createKey(const Context *ctx);
    virtual void initContext(Context *ctx);
    
    friend class RandomSubCacheStrategyFactory;
private:
    boost::int32_t getRand() const;
    
private:
    boost::int32_t max_value_;
};

class RandomSubCacheStrategyFactory : public SubCacheStrategyFactory {
    std::auto_ptr<SubCacheStrategy> create(const Config *config, const std::string &path) {
        boost::int32_t value;
        try {
            value = config->as<boost::int32_t>(path);
        }
        catch(const std::exception &e) {
            return std::auto_ptr<SubCacheStrategy>();
        }        
        return std::auto_ptr<SubCacheStrategy>(new RandomSubCacheStrategy(value));
    }
};

RandomSubCacheStrategy::RandomSubCacheStrategy(boost::int32_t max_value) :
    max_value_(max_value)
{}

void
RandomSubCacheStrategy::initContext(Context *ctx) {
    ctx->pageRandom(getRand());
}

std::string
RandomSubCacheStrategy::createKey(const Context *ctx) {
    return boost::lexical_cast<std::string>(ctx->pageRandom());
}

boost::int32_t
RandomSubCacheStrategy::getRand() const {
    boost::int32_t value = 0;
    if (max_value_ > 0) {
        boost::int32_t rand = random();
        value = max_value_ < RAND_MAX ? rand % (max_value_ + 1) : rand;
    }
    return value;
}

class CacheStrategyHandlersRegisterer {
public:
    CacheStrategyHandlersRegisterer() {
        CacheStrategyCollector::instance()->addPageStrategyHandler(
            "query", boost::shared_ptr<SubCacheStrategyFactory>(new QuerySubCacheStrategyFactory()));
        CacheStrategyCollector::instance()->addPageStrategyHandler(
            "cookie", boost::shared_ptr<SubCacheStrategyFactory>(new CookieSubCacheStrategyFactory()));
        CacheStrategyCollector::instance()->addPageStrategyHandler(
            "random", boost::shared_ptr<SubCacheStrategyFactory>(new RandomSubCacheStrategyFactory()));
    }
};

static CacheStrategyHandlersRegisterer reg_;

} // namespace xscript
