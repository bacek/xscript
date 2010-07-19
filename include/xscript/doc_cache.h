#ifndef _XSCRIPT_DOC_CACHE_H_
#define _XSCRIPT_DOC_CACHE_H_

#include <string>
#include <vector>

#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>

#include <xscript/cached_object.h>
#include <xscript/invoke_context.h>
#include <xscript/meta.h>
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
    CacheContext(CachedObject *obj, Context *ctx);
    CacheContext(CachedObject *obj, Context *ctx, bool allow_distributed);
    
    CachedObject* object() const;
    Context* context() const;
    bool allowDistributed() const;
    void allowDistributed(bool flag);
    
private:
    CachedObject* obj_;
    Context* ctx_;
    bool allow_distributed_;
};

class CacheData : private boost::noncopyable {
public:
    CacheData();
    virtual ~CacheData();
    
    virtual bool parse(const char *buf, boost::uint32_t size) = 0;
    virtual void serialize(std::string &buf) = 0;
    virtual void cleanup(Context *ctx) = 0;
};

class BlockCacheData : public CacheData {
public:
    BlockCacheData();
    BlockCacheData(XmlDocSharedHelper doc, boost::shared_ptr<MetaCore> meta);
    virtual ~BlockCacheData();
    
    virtual bool parse(const char *buf, boost::uint32_t size);
    virtual void serialize(std::string &buf);
    virtual void cleanup(Context *ctx);
    
    const XmlDocSharedHelper& doc() const;
    const boost::shared_ptr<MetaCore>& meta() const;
private:
    XmlDocSharedHelper doc_;
    boost::shared_ptr<MetaCore> meta_;
    static const boost::uint32_t SIGNATURE;
};

class PageCacheData : public CacheData, public BinaryWriter {
public:
    PageCacheData();
    PageCacheData(const char *buf, std::streamsize size);
    virtual ~PageCacheData();

    virtual bool parse(const char *buf, boost::uint32_t size);
    virtual void serialize(std::string &buf);
    virtual void cleanup(Context *ctx);
    
    void append(const char *buf, std::streamsize size);
    void addHeader(const std::string &name, const std::string &value);
    void expireTimeDelta(boost::uint32_t delta);
    
    virtual void write(std::ostream *os, const Response *response) const; // TODO: remove const
    virtual std::streamsize size() const;
    
    const std::string& etag() const;

private:
    void checkETag(const Response *response) const; // TODO: remove const

private:
    std::string data_;
    std::vector<std::pair<std::string, std::string> > headers_;
    boost::uint32_t expire_time_delta_;
    mutable std::string etag_; // TODO: remove mutable
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
    bool loadDocImpl(InvokeContext *invoke_ctx, CacheContext *cache_ctx,
            Tag &tag, boost::shared_ptr<CacheData> &cache_data);
    bool saveDocImpl(const InvokeContext *invoke_ctx, CacheContext *cache_ctx,
            const Tag &tag, const boost::shared_ptr<CacheData> &cache_data);
    
    bool allow(const DocCacheStrategy* strategy, const CacheContext *cache_ctx) const;
    
    class StatInfo;
    virtual void createUsageCounter(boost::shared_ptr<StatInfo> info) = 0;
    virtual std::string name() const = 0;
    
private:
    class DocCacheData;
    std::auto_ptr<DocCacheData> data_;
};

class DocCache : public DocCacheBase {
public:
    virtual ~DocCache();
    static DocCache* instance();
    
    boost::shared_ptr<BlockCacheData> loadDoc(
        InvokeContext *invoke_ctx, CacheContext *cache_ctx, Tag &tag);
    bool saveDoc(const InvokeContext *invoke_ctx, CacheContext *cache_ctx,
        const Tag &tag, const boost::shared_ptr<BlockCacheData> &cache_data);

protected:
    DocCache();
    
    virtual void createUsageCounter(boost::shared_ptr<StatInfo> info);
    virtual std::string name() const;
};

class PageCache : public DocCacheBase {
public:
    virtual ~PageCache();
    static PageCache* instance();

    boost::shared_ptr<PageCacheData> loadDoc(CacheContext *cache_ctx, Tag &tag);
    virtual bool saveDoc(CacheContext *cache_ctx, const Tag &tag,
            const boost::shared_ptr<PageCacheData> &cache_data);
    
    virtual void init(const Config *config);

    bool useETag() const;

protected:
    PageCache();
    
    virtual void createUsageCounter(boost::shared_ptr<StatInfo> info);
    virtual std::string name() const;

private:
    bool use_etag_;
};

} // namespace xscript

#endif // _XSCRIPT_DOC_CACHE_H_
