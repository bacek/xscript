#include "settings.h"

#include <boost/current_function.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include <limits>

#include "details/tag_param.h"
#include "xscript/context.h"
#include "xscript/logger.h"
#include "xscript/doc_cache.h"
#include "xscript/tagged_block.h"
#include "xscript/vhost_data.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

static const time_t CACHE_TIME_UNDEFINED = std::numeric_limits<time_t>::max();

TaggedBlock::TaggedBlock(const Extension *ext, Xml *owner, xmlNodePtr node) :
        Block(ext, owner, node), canonical_method_(),
        tagged_(false), cache_time_(CACHE_TIME_UNDEFINED), tag_position_(-1) {
}

TaggedBlock::~TaggedBlock() {
}

std::string
TaggedBlock::canonicalMethod(const Context *ctx) const {
    (void)ctx;
    return canonical_method_;
}

bool
TaggedBlock::tagged() const {
    return tagged_;
}

void
TaggedBlock::tagged(bool tagged) {
    tagged_ = tagged;
}

time_t
TaggedBlock::cacheTime() const {
    return cache_time_;
}

void
TaggedBlock::cacheTime(time_t cache_time) {
    log()->debug("%s, cache_time: %lu", BOOST_CURRENT_FUNCTION, static_cast<unsigned long>(cache_time));
    cache_time_ = cache_time;
}

InvokeResult
TaggedBlock::invokeInternal(Context *ctx) {

    log()->debug("%s", BOOST_CURRENT_FUNCTION);

    if (!tagged()) {
        return Block::invokeInternal(ctx);
    }

    if (!xsltName().empty() && ctx->noXsltPort()) {
        return Block::invokeInternal(ctx);
    }

    bool have_cached_doc = false;
    XmlDocHelper doc(NULL);
    Tag tag;
    try {
        have_cached_doc = DocCache::instance()->loadDoc(ctx, this, tag, doc);
    }
    catch (const std::exception &e) {
        log()->error("caught exception while fetching cached doc: %s", e.what());
    }

    if (!have_cached_doc) {
        return Block::invokeInternal(ctx);
    }
    
    if (Tag::UNDEFINED_TIME == tag.expire_time) {
        tag.modified = true;
        boost::any a(tag);
        XmlDocHelper newdoc = call(ctx, a);
        if (NULL != newdoc.get()) {
            return InvokeResult(processResponse(ctx, newdoc, a), true);
        }
    }

    evalXPath(ctx, doc);
    return InvokeResult(doc, true);
}

void
TaggedBlock::postCall(Context *ctx, const XmlDocHelper &doc, const boost::any &a) {

    log()->debug("%s, tagged: %d", BOOST_CURRENT_FUNCTION, static_cast<int>(tagged()));
    if (!tagged()) {
        return;
    }

    if (!xsltName().empty() && ctx->noXsltPort()) {
        return;
    }

    time_t now = time(NULL);
    Tag tag = boost::any_cast<Tag>(a);
    DocCache *cache = DocCache::instance();

    bool can_store = false;
    if (CACHE_TIME_UNDEFINED != cache_time_) {
        if (cache_time_ >= cache->minimalCacheTime()) {
            tag.expire_time = now + cache_time_;
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
        cache->saveDoc(ctx, this, tag, doc);
    }
}

void
TaggedBlock::processParam(std::auto_ptr<Param> p) {
    if (NULL == dynamic_cast<const TagParam*>(p.get())) {
        Block::processParam(p);
        return;
    }
    
    if (haveTagParam()) {
        throw std::runtime_error("duplicated tag param");
    }
    tagged(true);
    tag_position_ = params().size();
    
    const std::string& v = p->value();
    if (!v.empty()) {
        cacheTime(boost::lexical_cast<time_t>(v));
    }
}

void
TaggedBlock::postParse() {
    if (tagged()) {
        if ((CACHE_TIME_UNDEFINED != cache_time_) && (cache_time_ < DocCache::instance()->minimalCacheTime())) {
            tagged(false);
        }
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
    if (strncasecmp(name, "tag", sizeof("tag")) != 0) {
        return false;
    }
    
    if (tagged()) {
        throw std::runtime_error("duplicated tag");
    }
    
    if (strncasecmp(value, "yes", sizeof("yes")) != 0) {
        cache_time_ = boost::lexical_cast<time_t>(value); 
    }
    tagged(true);
    return true;
}

void
TaggedBlock::createCanonicalMethod(const char *prefix) {
    canonical_method_.clear();
    if (tagged()) {
        canonical_method_.append(prefix);
        const std::string &m = method();
        for (const char *str = m.c_str(); *str; ++str) {
            if (*str != '_' && *str != '-') {
                canonical_method_.append(1, tolower(*str));
            }
        }
    }
}

int
TaggedBlock::tagPosition() const {
    return tag_position_;
}

bool
TaggedBlock::haveTagParam() const {
    return tag_position_ >= 0;
}

bool
TaggedBlock::cacheTimeUndefined() const {
    return cache_time_ == CACHE_TIME_UNDEFINED;
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
    
    const std::vector<Param*> &block_params = params();
    for (unsigned int i = 0, n = block_params.size(); i < n; ++i) {
        key.append(1, ':').append(block_params[i]->asString(ctx));
    }
    const std::vector<Param*> &xslt_params = xsltParams();
    for (unsigned int i = 0, n = xslt_params.size(); i < n; ++i) {
        key.append(1, ':').append(xslt_params[i]->id());
        key.append(1, ':').append(xslt_params[i]->asString(ctx));
    }
    
    return key;
}

} // namespace xscript
