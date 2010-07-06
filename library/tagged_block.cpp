#include "settings.h"

#include <boost/current_function.hpp>
#include <boost/tokenizer.hpp>

#include <limits>

#include "xscript/context.h"
#include "xscript/doc_cache.h"
#include "xscript/logger.h"
#include "xscript/meta_block.h"
#include "xscript/policy.h"
#include "details/tag_param.h"
#include "xscript/tagged_block.h"
#include "xscript/vhost_data.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

struct TaggedBlock::TaggedBlockData {
    TaggedBlockData() : cache_level_(0), tag_position_(-1)
    {}
    
    ~TaggedBlockData() {
    }
    
    std::string canonical_method_;
    unsigned char cache_level_;
    int tag_position_;
    
    static const unsigned char FLAG_TAGGED = 1;
};

TaggedBlock::TaggedBlock(const Extension *ext, Xml *owner, xmlNodePtr node) :
        Block(ext, owner, node), tb_data_(new TaggedBlockData()) {
}

TaggedBlock::~TaggedBlock() {
}

bool
TaggedBlock::cacheTimeUndefined() const {
    return CachedObject::cacheTimeUndefined();
}

std::string
TaggedBlock::canonicalMethod(const Context *ctx) const {
    (void)ctx;
    return tb_data_->canonical_method_;
}

bool
TaggedBlock::tagged() const {
    return cacheLevel(TaggedBlockData::FLAG_TAGGED);
}

void
TaggedBlock::tagged(bool tagged) {
    if (tagged && !cacheTimeUndefined() &&
        cacheTime() < DocCache::instance()->minimalCacheTime()) {
        tagged = false;
    }
    cacheLevel(TaggedBlockData::FLAG_TAGGED, tagged);
}

bool
TaggedBlock::remote() const {
    return false;
}

void
TaggedBlock::cacheLevel(unsigned char type, bool value) {
    tb_data_->cache_level_ = value ?
        (tb_data_->cache_level_ | type) : (tb_data_->cache_level_ &= ~type);
}

bool
TaggedBlock::cacheLevel(unsigned char type) const {
    return tb_data_->cache_level_ & type;
}

time_t
TaggedBlock::cacheTime() const {
    return CachedObject::cacheTime();
}

void
TaggedBlock::cacheTime(time_t cache_time) {
    CachedObject::cacheTime(cache_time);
}

void
TaggedBlock::invokeInternal(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) {

    log()->debug("%s", BOOST_CURRENT_FUNCTION);

    if (!tagged()) {
        Block::invokeInternal(ctx, invoke_ctx);
        return;
    }

    if (xsltDefined() && ctx->noXsltPort()) {
        Block::invokeInternal(ctx, invoke_ctx);
        return;
    }
    
    if (!Policy::instance()->allowCaching(ctx.get(), this)) {
        Block::invokeInternal(ctx, invoke_ctx);
        return;
    }

    invoke_ctx->meta()->cacheParamsWritable(cacheTimeUndefined());

    Tag cache_tag(true, 1, 1); // fake undefined Tag
    try {
        CacheContext cache_ctx(this, ctx.get(), this->allowDistributed());
        boost::shared_ptr<BlockCacheData> cache_data = 
            DocCache::instance()->loadDoc(invoke_ctx.get(), &cache_ctx, cache_tag);
        if (cache_data.get() && cache_data->doc().get()) {
            invoke_ctx->resultDoc(cache_data->doc());
            if (metaBlock()) {
                invoke_ctx->meta()->setCore(cache_data->meta());
                invoke_ctx->meta()->setCacheParams(
                    cache_tag.expire_time, cache_tag.last_modified);
                callMetaAfterCacheLoadLua(ctx, invoke_ctx);
                if (cacheTimeUndefined()) {
                    cache_tag.expire_time = invoke_ctx->meta()->getExpireTime();
                    cache_tag.last_modified = invoke_ctx->meta()->getLastModified();
                }
            }
        }
        else {
            cache_tag.expire_time = 1;
            cache_tag.last_modified = 1;
        }
    }
    catch (const MetaInvokeError &e) {
        log()->error("caught meta exception while fetching and processing cached doc: %s", e.what());
        if (processCachedDoc(ctx, invoke_ctx, cache_tag)) {
            throw e;
        }
        return;
    }
    catch (const std::exception &e) {
        log()->error("caught exception while fetching and processing cached doc: %s", e.what());
    }

    processCachedDoc(ctx, invoke_ctx, cache_tag);
}

// return true if cache copy processed
bool
TaggedBlock::processCachedDoc(boost::shared_ptr<Context> ctx,
    boost::shared_ptr<InvokeContext> invoke_ctx, const Tag &tag) {

    if (tag.expired()) {
        invoke_ctx->meta()->reset();
        invoke_ctx->meta()->cacheParamsWritable(cacheTimeUndefined());
        invoke_ctx->resultDoc(XmlDocSharedHelper());
        Block::invokeInternal(ctx, invoke_ctx);
        return false;
    }

    if (Tag::UNDEFINED_TIME == tag.expire_time) {
        boost::shared_ptr<Meta> meta = invoke_ctx->meta();
        invoke_ctx->setMeta(boost::shared_ptr<Meta>(new Meta));
        invoke_ctx->haveCachedCopy(true);
        invoke_ctx->tag(tag);
        XmlDocSharedHelper doc = invoke_ctx->resultDoc();
        call(ctx, invoke_ctx);
        if (invoke_ctx->tag().modified) {
            if (NULL == invoke_ctx->resultDoc().get()) {
                log()->error("Got empty document in tagged block. Cached copy used");
            }
            else {
                processResponse(ctx, invoke_ctx);
                return false;
            }
        }
        invoke_ctx->resultDoc(doc);
        invoke_ctx->setMeta(meta);
    }

    invoke_ctx->resultType(InvokeContext::SUCCESS);
    return true;
}

void
TaggedBlock::postCall(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) {

    log()->debug("%s, tagged: %d", BOOST_CURRENT_FUNCTION, static_cast<int>(tagged()));
    if (!tagged() || !invoke_ctx->success()) {
        return;
    }

    if (xsltDefined() && ctx->noXsltPort()) {
        return;
    }
    
    if (!Policy::instance()->allowCaching(ctx.get(), this)) {
        return;
    }

    try {
        callMetaBeforeCacheSaveLua(ctx, invoke_ctx);
    }
    catch (const InvokeError &e) {
        throw MetaInvokeError(e.what());
    }
    catch (const std::exception &e) {
        throw MetaInvokeError(e.what());
    }

    time_t now = time(NULL);
    DocCache *cache = DocCache::instance();
    Tag tag = invoke_ctx->tag();
    
    time_t expire_time = invoke_ctx->meta()->getExpireTime();
    if (Meta::undefinedExpireTime() != expire_time) {
        tag.expire_time = expire_time;
    }
    else {
        boost::shared_ptr<Context> local_ctx = invoke_ctx->getLocalContext();
        if (local_ctx.get() && !local_ctx->expireDeltaUndefined()) {
            tag.expire_time = tag.last_modified + local_ctx->expireDelta();
        }
    }

    time_t last_modified = invoke_ctx->meta()->getLastModified();
    if (Meta::undefinedLastModified() != last_modified) {
        tag.last_modified = last_modified;
    }

    bool can_store = false;
    if (!cacheTimeUndefined()) {
        if (cacheTime() >= cache->minimalCacheTime()) {
            tag.expire_time = now + cacheTime();
            can_store = true;
        }
    }
    else if (Tag::UNDEFINED_TIME == tag.expire_time) {
        can_store = Tag::UNDEFINED_TIME != tag.last_modified && !invoke_ctx->haveCachedCopy();
    }
    else {
        can_store = tag.expire_time >= now + cache->minimalCacheTime();
    }

    bool res = false;
    if (can_store) {
        CacheContext cache_ctx(this, ctx.get(), this->allowDistributed());
        boost::shared_ptr<MetaCore> meta;
        if (metaBlock()) {
            meta = invoke_ctx->meta()->getCore();
        }
        boost::shared_ptr<BlockCacheData> cache_data(
            new BlockCacheData(invoke_ctx->resultDoc(), meta));
        res = cache->saveDoc(invoke_ctx.get(), &cache_ctx, tag, cache_data);
    }

    invoke_ctx->meta()->setCacheParams(
        res ? tag.expire_time : Meta::undefinedExpireTime(),
        res ? tag.last_modified : Meta::undefinedLastModified());
}

void
TaggedBlock::parseParamNode(const xmlNodePtr node) {
    std::auto_ptr<Param> param = createParam(node);
    if (NULL == dynamic_cast<const TagParam*>(param.get())) {
        addParam(param);
        return;
    }
    
    if (haveTagParam()) {
        throw std::runtime_error("duplicated tag param");
    }
    tb_data_->tag_position_ = params().size();
    
    const std::string& v = param->value();
    if (!v.empty()) {
        try {
            cacheTime(boost::lexical_cast<time_t>(v));
        }
        catch(const boost::bad_lexical_cast &e) {
            throw std::runtime_error(
                std::string("cannot parse tag param value: ") + v);
        }            
    }
    
    tagged(true);
}

void
TaggedBlock::postParse() {
    Block::postParse();
    MetaBlock *meta_block = metaBlock();
    if (meta_block && meta_block->haveCachedLua() && !cacheTimeUndefined()) {
        throw std::runtime_error("Meta cache lua sections allowed for tag=yes only");
    }
}

void
TaggedBlock::property(const char *name, const char *value) {
    if (!propertyInternal(name , value)) {
        Block::property(name, value);
    }
}

bool
TaggedBlock::propertyInternal(const char *name, const char *value) {
    if (strncasecmp(name, "no-cache", sizeof("no-cache")) == 0) {
        Policy::instance()->processCacheLevel(this, value);
    }
    else if (CachedObject::checkProperty(name, value)) {        
    }
    else if (strncasecmp(name, "tag", sizeof("tag")) == 0) {
        if (tagged()) {
            throw std::runtime_error("duplicated tag");
        }
        
        if (strncasecmp(value, "yes", sizeof("yes")) != 0) {
            try {
                cacheTime(boost::lexical_cast<time_t>(value));
            }
            catch(const boost::bad_lexical_cast &e) {
                throw std::runtime_error(
                    std::string("cannot parse tag value: ") + value);
            }
        }
        tagged(true);
    }
    else {
        return false;
    }
    
    return true;
}

void
TaggedBlock::createCanonicalMethod(const char *prefix) {
    tb_data_->canonical_method_.clear();
    if (tagged()) {
        tb_data_->canonical_method_.append(prefix);
        const std::string &m = method();
        for (const char *str = m.c_str(); *str; ++str) {
            if (*str != '_' && *str != '-') {
                tb_data_->canonical_method_.append(1, tolower(*str));
            }
        }
    }
}

int
TaggedBlock::tagPosition() const {
    return tb_data_->tag_position_;
}

bool
TaggedBlock::haveTagParam() const {
    return tb_data_->tag_position_ >= 0;
}

std::string
TaggedBlock::info(const Context *ctx) const {
    
    std::string info = canonicalMethod(ctx);
    
    const std::string &xslt = xsltNameRaw();
    if (!xslt.empty()) {        
        info.append(" | Xslt: ");
        info.append(xslt);
    }

    const std::vector<Param*> &block_params = params();
    if (!block_params.empty()) {
        info.append(" | Params: ");
        for (unsigned int i = 0, n = block_params.size(); i < n; ++i) {
            if (i > 0) {
                info.append(", ");
            }
            info.append(block_params[i]->type());
            const std::string& value =  block_params[i]->value();
            if (!value.empty()) {
                info.push_back('(');
                info.append(value);
                info.push_back(')');
            }            
        }
    }
    
    const std::vector<Param*> &xslt_params = xsltParams();
    if (!xslt_params.empty()) {
        info.append(" | Xslt-Params: ");
        for (unsigned int i = 0, n = xslt_params.size(); i < n; ++i) {
            if (i > 0) {
                info.append(", ");
            }
            info.append(xslt_params[i]->type());
            const std::string& value =  xslt_params[i]->value();
            if (!value.empty()) {
                info.push_back('(');
                info.append(value);
                info.push_back(')');
            }            
        }
    }
    
    if (tagged()) {
        info.append(" | Cache-time: ");
        if (cacheTimeUndefined()) {
            info.append("undefined");    
        }
        else {
            info.append(boost::lexical_cast<std::string>(cacheTime()));
        }
    }
    
    return info;
}

std::string
TaggedBlock::createTagKey(const Context *ctx, const InvokeContext *invoke_ctx) const {
    std::string key(processMainKey(ctx, invoke_ctx));
    key.push_back('|');
    key.append(processParamsKey(ctx));
    return key;
}

std::string
TaggedBlock::processMainKey(const Context *ctx, const InvokeContext *invoke_ctx) const {
    std::string key;
    const std::string &xslt = invoke_ctx->xsltName();
    if (!xslt.empty()) {
        key.assign(xslt);
        key.push_back('|');
        key.append(fileModifiedKey(xslt));
        key.push_back('|');
    }
    key.append(boost::lexical_cast<std::string>(cacheTime()));
    key.push_back('|');
    key.append(canonicalMethod(ctx));
    key.push_back('|');
    key.append(paramsIdKey(xsltParams(), ctx));
    if (metaBlock()) {
        key.push_back('|');
        key.append(metaBlock()->getTagKey());
    }
    return key;
}

std::string
TaggedBlock::processParamsKey(const Context *ctx) const {
    return paramsKey(params(), ctx);
}

void
TaggedBlock::callMetaLua(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) {
    Block::callMetaLua(ctx, invoke_ctx);
}

void
TaggedBlock::callMetaBeforeCacheSaveLua(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) {
    MetaBlock* meta_block = metaBlock();
    if (meta_block) {
        meta_block->callBeforeCacheSaveLua(ctx, invoke_ctx);
    }
}

void
TaggedBlock::callMetaAfterCacheLoadLua(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) {
    MetaBlock* meta_block = metaBlock();
    if (meta_block) {
        meta_block->callAfterCacheLoadLua(ctx, invoke_ctx);
    }
}

} // namespace xscript
