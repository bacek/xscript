#ifndef _XSCRIPT_DOC_CACHE_H_
#define _XSCRIPT_DOC_CACHE_H_

#include <vector>
#include <string>

#include <xscript/average_counter.h>
#include <xscript/control_extension.h>
#include <xscript/stat_builder.h>
#include <xscript/tagged_cache_usage_counter.h>
#include <xscript/xml_helpers.h>

namespace xscript {

class Context;
class TaggedBlock;
class Tag;
class DocCacheStrategy;

class Block;
class Object;
class Extension;
class Xml;

/**
 * Cache result of block invokations using sequence of different strategies.
 */

class DocCacheBase {
public:
    DocCacheBase();
    virtual ~DocCacheBase();

    time_t minimalCacheTime() const;

    bool loadDoc(const Context *ctx, const Object *obj, Tag &tag, XmlDocHelper &doc);
    bool saveDoc(const Context *ctx, const Object *obj, const Tag& tag, const XmlDocHelper &doc);

    virtual void init(const Config *config);
    void addStrategy(DocCacheStrategy* strategy, const std::string& name);

    /**
     * Create block to output statistic in xscript page.
     */
    std::auto_ptr<Block> createBlock(const ControlExtension *ext, Xml *owner, xmlNodePtr node);

    /**
     * Create aggregate report. Caller must free result.
     */
    XmlDocHelper createReport() const;
protected:
    struct StatInfo {
        std::auto_ptr<StatBuilder> statBuilder_;
        std::auto_ptr<AverageCounter> hitCounter_;
        std::auto_ptr<AverageCounter> missCounter_;
        std::auto_ptr<AverageCounter> saveCounter_;
        std::auto_ptr<TaggedCacheUsageCounter> usageCounter_;
    };
    
protected:    
    virtual void createUsageCounter(boost::shared_ptr<StatInfo> info) = 0;
    virtual std::string name() const = 0;
    
private:   
    std::vector<std::string> strategiesOrder_;
    typedef std::vector<std::pair<DocCacheStrategy*, boost::shared_ptr<StatInfo> > > StrategyMap;
    StrategyMap strategies_;
};

class DocCache : public DocCacheBase {
public:
    virtual ~DocCache();
    
    static DocCache* instance() {
        static DocCache cache;
        return &cache;
    }

protected:
    DocCache();
    
    virtual void createUsageCounter(boost::shared_ptr<StatInfo> info);
    virtual std::string name() const;
};

class PageCache : public DocCacheBase {
public:
    virtual ~PageCache();
    
    static PageCache* instance() {
        static PageCache cache;
        return &cache;
    }

protected:
    PageCache();
    
    virtual void createUsageCounter(boost::shared_ptr<StatInfo> info);
    virtual std::string name() const;
};

} // namespace xscript

#endif // _XSCRIPT_DOC_CACHE_H_
