#ifndef _XSCRIPT_TYPED_CACHE_H_
#define _XSCRIPT_TYPED_CACHE_H_

#include <string>

#include "xscript/cached_object.h"
#include "xscript/doc_cache.h"
#include "xscript/typed_map.h"

namespace xscript {


class TypedCachedObject : public CachedObject {
public:
    explicit TypedCachedObject(const std::string &key);
    virtual ~TypedCachedObject();

    virtual std::string createTagKey(const Context *ctx, const InvokeContext *invoke_ctx) const {
        (void)ctx;
        (void)invoke_ctx;
        return key_;
    }
    virtual bool allowDistributed() const { return false; }

private:
    std::string key_;
};


class TypedCacheData : public CacheData {
public:
    TypedCacheData();
    TypedCacheData(const std::string &key, TypedValue &value);
    virtual ~TypedCacheData();

    virtual xmlDocPtr docPtr() const { return NULL; }

    virtual bool parse(const char *buf, boost::uint32_t size) {
        (void)buf;
        (void)size;
        return false;
    }
    virtual bool serialize(std::string &buf) { (void)buf; return false; }
    virtual void cleanup(Context *ctx) { (void)ctx; }

    const std::string& key() const { return key_; }
    const TypedValue& value() const { return value_; }

private:
    std::string key_;
    TypedValue value_;
};


class TypedCache : public DocCacheBase {
public:
    virtual ~TypedCache();
    static TypedCache* instance();

    boost::shared_ptr<TypedCacheData> load(CacheContext *cache_ctx, Tag &tag);

    bool save(CacheContext *cache_ctx, const Tag &tag,
            const boost::shared_ptr<TypedCacheData> &cache_data);

protected:
    TypedCache();

    virtual void createUsageCounter(boost::shared_ptr<StatInfo> info);
    virtual std::string name() const;
};

            
} // namespace xscript

#endif // _XSCRIPT_TYPED_CACHE_H_
