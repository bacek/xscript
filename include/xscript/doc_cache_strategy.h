#ifndef _XSCRIPT_DOC_CACHE_STRATEGY_H_
#define _XSCRIPT_DOC_CACHE_STRATEGY_H_

#include <memory>
#include <vector>
#include <boost/utility.hpp>

#include <xscript/component.h>
#include <xscript/xml_helpers.h>

namespace xscript {

class CachedObject;
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
    
    virtual bool distributed() const;

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
    
    void addStrategy(DocCacheStrategy* strategy, const std::string& name);
private:
    std::vector<std::pair<DocCacheStrategy*, std::string> > strategies_;
};

} // namespace xscript

#endif // _XSCRIPT_DOC_CACHE_STRATEGY_H_
