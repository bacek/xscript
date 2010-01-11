#ifndef _XSCRIPT_DOC_CACHE_STRATEGY_H_
#define _XSCRIPT_DOC_CACHE_STRATEGY_H_

#include <memory>
#include <vector>
#include <boost/utility.hpp>

#include <xscript/cached_object.h>
#include <xscript/cache_strategy.h>
#include <xscript/component.h>
#include <xscript/xml_helpers.h>

namespace xscript {

class Context;
class StatBuilder;
class Tag;
class TaggedBlock;

class TagKey : private boost::noncopyable {
public:
    TagKey();
    virtual ~TagKey();
    virtual const std::string& asString() const = 0;
};


class DocCacheStrategy {
public:
    DocCacheStrategy();
    virtual ~DocCacheStrategy();

    virtual void init(const Config *config);

    virtual time_t minimalCacheTime() const = 0;
    virtual std::string name() const = 0;
    virtual std::auto_ptr<TagKey> createKey(const Context *ctx, const CachedObject *obj) const = 0;

    virtual bool loadDoc(const TagKey *key, Tag &tag, XmlDocSharedHelper &doc, bool need_copy);
    virtual bool saveDoc(const TagKey *key, const Tag& tag, const XmlDocSharedHelper &doc, bool need_copy);
    
    virtual void fillStatBuilder(StatBuilder *builder);
    
    virtual CachedObject::Strategy strategy() const = 0;

protected:
    virtual bool loadDocImpl(const TagKey *key, Tag &tag, XmlDocSharedHelper &doc, bool need_copy) = 0;
    virtual bool saveDocImpl(const TagKey *key, const Tag& tag, const XmlDocSharedHelper &doc, bool need_copy) = 0;
    
    virtual void insert2Cache(const std::string &no_cache);
};

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
    std::vector<std::pair<DocCacheStrategy*, std::string> > strategies_;
    
    typedef std::map<std::string, boost::shared_ptr<SubCacheStrategyFactory> > HandlerMapType;
    HandlerMapType block_strategy_handlers_;
    HandlerMapType page_strategy_handlers_;
    
    typedef std::map<std::string, boost::shared_ptr<CacheStrategy> > StrategyMapType;
    StrategyMapType block_cache_strategies_;
    StrategyMapType page_cache_strategies_;
};

} // namespace xscript

#endif // _XSCRIPT_DOC_CACHE_STRATEGY_H_
