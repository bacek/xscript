#ifndef _XSCRIPT_DOC_CACHE_H_
#define _XSCRIPT_DOC_CACHE_H_

#include <string>

#include <boost/shared_ptr.hpp>

#include <xscript/cached_object.h>
#include <xscript/xml_helpers.h>

namespace xscript {

class CachedObject;
class Config;
class Context;
class DocCacheStrategy;
class Tag;

/**
 * Cache result of block invocations using sequence of different strategies.
 */

class CacheContext {
public:
    CacheContext(const CachedObject *obj);
    CacheContext(const CachedObject *obj, bool allow_distributed);
    
    const CachedObject* object() const;
    bool allowDistributed() const;
    void allowDistributed(bool flag);
    
private:
    const CachedObject* obj_;
    bool allow_distributed_;
};

class DocCacheBase {
public:
    DocCacheBase();
    virtual ~DocCacheBase();

    time_t minimalCacheTime() const;

    virtual bool loadDoc(const Context *ctx, const CacheContext *cache_ctx, Tag &tag, XmlDocSharedHelper &doc);
    virtual bool saveDoc(const Context *ctx, const CacheContext *cache_ctx, const Tag &tag, const XmlDocSharedHelper &doc);

    virtual void init(const Config *config);
    void addStrategy(DocCacheStrategy *strategy, const std::string &name);
    
    static bool checkTag(const Context *ctx, const Tag &tag, const char *operation);
    
protected:
    bool loadDocImpl(const Context *ctx, const CacheContext *cache_ctx, Tag &tag, XmlDocSharedHelper &doc, bool need_copy);
    bool saveDocImpl(const Context *ctx, const CacheContext *cache_ctx, const Tag &tag, const XmlDocSharedHelper &doc, bool need_copy);

    bool allow(const DocCacheStrategy* strategy, const CacheContext *cache_ctx) const;
    
    class StatInfo;
    virtual void createUsageCounter(boost::shared_ptr<StatInfo> info) = 0;
    virtual std::string name() const = 0;
    
private:
    class DocCacheData;
    // workaround for woody
    friend class DocCacheData;
    DocCacheData *data_;
};

class DocCache : public DocCacheBase {
public:
    virtual ~DocCache();
    static DocCache* instance();

protected:
    DocCache();
    
    virtual void createUsageCounter(boost::shared_ptr<StatInfo> info);
    virtual std::string name() const;
};

class PageCache : public DocCacheBase {
public:
    virtual ~PageCache();
    static PageCache* instance();

    virtual bool loadDoc(const Context *ctx, const CacheContext *cache_ctx, Tag &tag, XmlDocSharedHelper &doc);
    virtual bool saveDoc(const Context *ctx, const CacheContext *cache_ctx, const Tag &tag, const XmlDocSharedHelper &doc);
    
protected:
    PageCache();
    
    virtual void createUsageCounter(boost::shared_ptr<StatInfo> info);
    virtual std::string name() const;
};

} // namespace xscript

#endif // _XSCRIPT_DOC_CACHE_H_
