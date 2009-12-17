#include "settings.h"

#include <boost/current_function.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/tokenizer.hpp>

#include <limits>

#include "xscript/context.h"
#include "xscript/doc_cache.h"
#include "xscript/logger.h"
#include "xscript/policy.h"
#include "details/tag_param.h"
#include "xscript/tagged_block.h"
#include "xscript/vhost_data.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

static const time_t CACHE_TIME_UNDEFINED = std::numeric_limits<time_t>::max();

struct TaggedBlock::TaggedBlockData {
    TaggedBlockData() : cache_level_(0), cache_time_(CACHE_TIME_UNDEFINED), tag_position_(-1)
    {}
    
    ~TaggedBlockData() {
    }
    
    std::string canonical_method_;
    unsigned char cache_level_;
    time_t cache_time_;
    int tag_position_;
    
    static const unsigned char FLAG_TAGGED = 1;
};

TaggedBlock::TaggedBlock(const Extension *ext, Xml *owner, xmlNodePtr node) :
        Block(ext, owner, node), tb_data_(new TaggedBlockData()) {
}

TaggedBlock::~TaggedBlock() {
    delete tb_data_;
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
    if (tagged &&
        CACHE_TIME_UNDEFINED != tb_data_->cache_time_ &&
        tb_data_->cache_time_ < DocCache::instance()->minimalCacheTime()) {
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
    return tb_data_->cache_time_;
}

void
TaggedBlock::cacheTime(time_t cache_time) {
    log()->debug("%s, cache_time: %lu", BOOST_CURRENT_FUNCTION, static_cast<unsigned long>(cache_time));
    tb_data_->cache_time_ = cache_time;
}

void
TaggedBlock::invokeInternal(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> &invoke_ctx) {

    log()->debug("%s", BOOST_CURRENT_FUNCTION);

    if (!tagged()) {
        Block::invokeInternal(ctx, invoke_ctx);
        return;
    }

    if (!xsltName().empty() && ctx->noXsltPort()) {
        Block::invokeInternal(ctx, invoke_ctx);
        return;
    }
    
    if (!Policy::allowCaching(ctx.get(), this)) {
        Block::invokeInternal(ctx, invoke_ctx);
        return;
    }

    bool have_cached_doc = false;
    XmlDocSharedHelper doc;
    Tag cache_tag;
    try {
        CacheContext cache_ctx(this, this->allowDistributed());
        have_cached_doc = DocCache::instance()->loadDoc(ctx.get(), &cache_ctx, cache_tag, doc);
    }
    catch (const std::exception &e) {
        log()->error("caught exception while fetching cached doc: %s", e.what());
    }
   
    if (!have_cached_doc) {
        Block::invokeInternal(ctx, invoke_ctx);
        return;
    }
    
    if (Tag::UNDEFINED_TIME == cache_tag.expire_time) {
        invoke_ctx->haveCachedCopy(true);
        invoke_ctx->tag(cache_tag);
        XmlDocSharedHelper newdoc(new XmlDocHelper(call(ctx, invoke_ctx)));
        
        if (invoke_ctx->tag().modified) {
            if (NULL == newdoc->get()) {
                log()->error("Got empty document in tagged block. Cached copy used");
            }
            else {
                invoke_ctx->resultDoc(newdoc);
                processResponse(ctx, invoke_ctx);
                return;
            }
        }
    }
    
    invoke_ctx->resultType(InvokeContext::SUCCESS);
    invoke_ctx->resultDoc(doc);
    evalXPath(ctx.get(), doc);
}

void
TaggedBlock::postCall(Context *ctx, InvokeContext *invoke_ctx) {

    log()->debug("%s, tagged: %d", BOOST_CURRENT_FUNCTION, static_cast<int>(tagged()));
    if (!tagged() || !invoke_ctx->success()) {
        return;
    }

    if (!xsltName().empty() && ctx->noXsltPort()) {
        return;
    }
    
    if (!Policy::allowCaching(ctx, this)) {
        return;
    }
    
    time_t now = time(NULL);
    DocCache *cache = DocCache::instance();
    Tag tag = invoke_ctx->tag();

    bool can_store = false;
    if (CACHE_TIME_UNDEFINED != tb_data_->cache_time_) {
        if (tb_data_->cache_time_ >= cache->minimalCacheTime()) {
            tag.expire_time = now + tb_data_->cache_time_;
            can_store = true;
        }
    }
    else if (Tag::UNDEFINED_TIME == tag.expire_time) {
        can_store = Tag::UNDEFINED_TIME != tag.last_modified;
    }
    else {
        can_store = tag.expire_time >= now + cache->minimalCacheTime();
    }

    if (can_store) {
        CacheContext cache_ctx(this, this->allowDistributed());
        cache->saveDoc(ctx, &cache_ctx, tag, invoke_ctx->resultDoc());
    }
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
        Policy::processCacheLevel(this, value);
    }
    else if (CachedObject::checkProperty(name, value)) {        
    }
    else if (strncasecmp(name, "tag", sizeof("tag")) == 0) {
        if (tagged()) {
            throw std::runtime_error("duplicated tag");
        }
        
        if (strncasecmp(value, "yes", sizeof("yes")) != 0) {
            try {
                tb_data_->cache_time_ = boost::lexical_cast<time_t>(value);
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

bool
TaggedBlock::cacheTimeUndefined() const {
    return tb_data_->cache_time_ == CACHE_TIME_UNDEFINED;
}

std::string
TaggedBlock::info(const Context *ctx) const {
    
    std::string info = canonicalMethod(ctx);
    
    const std::string& xslt = xsltName();
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
TaggedBlock::createTagKey(const Context *ctx) const {
    return processMainKey(ctx) + processParamsKey(ctx);
}

std::string
TaggedBlock::processMainKey(const Context *ctx) const {
    std::string key;
    
    const std::string& xslt = xsltName();
    if (!xslt.empty()) {
        key.assign(xslt);
        key.push_back('|');
        
        namespace fs = boost::filesystem;
        fs::path path(xslt);
        if (fs::exists(path) && !fs::is_directory(path)) {
            key.append(boost::lexical_cast<std::string>(fs::last_write_time(path)));
            key.push_back('|');
        }
        else {
            throw std::runtime_error("Cannot stat stylesheet " + xslt);
        }
    }
    key.append(boost::lexical_cast<std::string>(cacheTime()));
    key.push_back('|');
    key.append(canonicalMethod(ctx));
    
    const std::vector<Param*> &xslt_params = xsltParams();
    for (unsigned int i = 0, n = xslt_params.size(); i < n; ++i) {
        key.append(1, ':').append(xslt_params[i]->id());
        key.append(1, ':').append(xslt_params[i]->asString(ctx));
    }
    
    return key;
}

std::string
TaggedBlock::processParamsKey(const Context *ctx) const {
    std::string key;
    const std::vector<Param*> &block_params = params();
    for (unsigned int i = 0, n = block_params.size(); i < n; ++i) {
        key.append(1, ':').append(block_params[i]->asString(ctx));
    }
    return key;
}

} // namespace xscript
