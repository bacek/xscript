#ifndef _XSCRIPT_DOC_CACHE_H_
#define _XSCRIPT_DOC_CACHE_H_

#include <string>

#include <xscript/xml_helpers.h>

namespace xscript {

class Context;
class DocCacheStrategy;
class Object;
class Tag;

/**
 * Cache result of block invokations using sequence of different strategies.
 */

class DocCacheBase {
public:
    DocCacheBase();
    virtual ~DocCacheBase();

    time_t minimalCacheTime() const;

    bool loadDoc(const Context *ctx, const Object *obj, Tag &tag, XmlDocHelper &doc);
    bool saveDoc(const Context *ctx, const Object *obj, const Tag &tag, const XmlDocHelper &doc);

    virtual void init(const Config *config);
    void addStrategy(DocCacheStrategy *strategy, const std::string &name);
    
protected:
    class StatInfo;
    virtual void createUsageCounter(boost::shared_ptr<StatInfo> info) = 0;
    virtual std::string name() const = 0;
    
private:
    class DocCacheData;
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

protected:
    PageCache();
    
    virtual void createUsageCounter(boost::shared_ptr<StatInfo> info);
    virtual std::string name() const;
};

} // namespace xscript

#endif // _XSCRIPT_DOC_CACHE_H_
