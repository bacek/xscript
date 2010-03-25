#ifndef _XSCRIPT_DOC_CACHE_H_
#define _XSCRIPT_DOC_CACHE_H_

#include <string>
#include <vector>

#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>

#include <xscript/cached_object.h>
#include <xscript/writer.h>
#include <xscript/xml_helpers.h>

namespace xscript {

class CachedObject;
class Config;
class Context;
class DocCacheStrategy;
class InvokeContext;
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

class CacheData {
public:
    CacheData();
    virtual ~CacheData();
    
    virtual bool parse(const char *buf, boost::uint32_t size) = 0;
    virtual void serialize(std::string &buf) = 0;
};

class BlockCacheData : public CacheData {
public:
    BlockCacheData();
    BlockCacheData(XmlDocSharedHelper doc);
    ~BlockCacheData();
    
    virtual bool parse(const char *buf, boost::uint32_t size);
    virtual void serialize(std::string &buf);
    
    const XmlDocSharedHelper& doc() const;
private:
    XmlDocSharedHelper doc_;
    static const boost::uint32_t SIGNATURE;
};

class PageCacheData : public CacheData, public BinaryWriter {
public:
    PageCacheData();
    PageCacheData(const char *buf, std::streamsize size);
    virtual ~PageCacheData();

    virtual bool parse(const char *buf, boost::uint32_t size);
    virtual void serialize(std::string &buf);
    
    void append(const char *buf, std::streamsize size);
    void addHeader(const std::string &name, const std::string &value);
    void expireTimeDelta(boost::uint32_t delta);
    
    virtual void write(std::ostream *os) const;
    virtual std::streamsize size() const;
    
private:
    std::string data_;
    std::vector<std::pair<std::string, std::string> > headers_;
    boost::uint32_t expire_time_delta_;
    static const boost::uint32_t SIGNATURE;
};

class DocCacheBase {
public:
    DocCacheBase();
    virtual ~DocCacheBase();

    time_t minimalCacheTime() const;

    virtual void init(const Config *config);
    void addStrategy(DocCacheStrategy *strategy, const std::string &name);
    
    static bool checkTag(const Context *ctx, const Tag &tag, const char *operation);
    
protected:
    bool loadDocImpl(const Context *ctx, InvokeContext *invoke_ctx,
            const CacheContext *cache_ctx, Tag &tag, boost::shared_ptr<CacheData> &cache_data);
    bool saveDocImpl(const Context *ctx, const InvokeContext *invoke_ctx,
            const CacheContext *cache_ctx, const Tag &tag, const boost::shared_ptr<CacheData> &cache_data);
    
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
    
    boost::shared_ptr<BlockCacheData> loadDoc(const Context *ctx, InvokeContext *invoke_ctx,
            const CacheContext *cache_ctx, Tag &tag);
    bool saveDoc(const Context *ctx, const InvokeContext *invoke_ctx,
            const CacheContext *cache_ctx, const Tag &tag, const boost::shared_ptr<BlockCacheData> &cache_data);

protected:
    DocCache();
    
    virtual void createUsageCounter(boost::shared_ptr<StatInfo> info);
    virtual std::string name() const;
};

class PageCache : public DocCacheBase {
public:
    virtual ~PageCache();
    static PageCache* instance();

    boost::shared_ptr<PageCacheData> loadDoc(const Context *ctx, const CacheContext *cache_ctx, Tag &tag);
    virtual bool saveDoc(const Context *ctx, const CacheContext *cache_ctx, const Tag &tag,
            const boost::shared_ptr<PageCacheData> &cache_data);
    
protected:
    PageCache();
    
    virtual void createUsageCounter(boost::shared_ptr<StatInfo> info);
    virtual std::string name() const;
};

} // namespace xscript

#endif // _XSCRIPT_DOC_CACHE_H_
