#include "settings.h"

#include <boost/current_function.hpp>
#include <limits>

#include "xscript/context.h"
#include "xscript/logger.h"
#include "xscript/doc_cache.h"
#include "xscript/tagged_block.h"
#include "xscript/vhost_data.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

static const time_t CACHE_TIME_MINIMUM = 5;
static const time_t CACHE_TIME_UNDEFINED = std::numeric_limits<time_t>::max();

TaggedBlock::TaggedBlock(const Extension *ext, Xml *owner, xmlNodePtr node) :
        Block(ext, owner, node), canonical_method_(), tagged_(false), cache_time_(CACHE_TIME_UNDEFINED) {
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

    const Server* server = VirtualHostData::instance()->getServer();
    if (!xsltName().empty() && server && !server->needApplyPerblockStylesheet(ctx->request())) {
        return Block::invokeInternal(ctx);
    }

    try {
        bool have_cached_doc = false;
        XmlDocHelper doc(NULL);
        try {
            Tag tag;
            have_cached_doc = DocCache::instance()->loadDoc(ctx, this, tag, doc);

            if (have_cached_doc && Tag::UNDEFINED_TIME == tag.expire_time) {

                tag.modified = true;

                boost::any a(tag);
                XmlDocHelper newdoc = call(ctx, a);
                tag = boost::any_cast<Tag>(a);

                if (NULL != newdoc.get()) {
                    return InvokeResult(processResponse(ctx, newdoc, a), true);
                }

                if (tag.modified) {
                    log()->error("Got empty document in tagged block. Cached copy used");
                }
                else {
                    log()->debug("Got empty document and tag not modified. Cached copy used");
                }
            }
        }
        catch (const std::exception &e) {
            log()->error("caught exception while invoking tagged block: %s", e.what());
        }

        if (have_cached_doc) {
            evalXPath(ctx, doc);
            return InvokeResult(doc, true);
        }

        return Block::invokeInternal(ctx);
    }
    catch (const std::exception &e) {
        return InvokeResult(errorResult(e.what()), false);
    }
}

void
TaggedBlock::postCall(Context *ctx, const XmlDocHelper &doc, const boost::any &a) {

    log()->debug("%s, tagged: %d", BOOST_CURRENT_FUNCTION, static_cast<int>(tagged()));
    if (!tagged()) {
        return;
    }

    const Server* server = VirtualHostData::instance()->getServer();
    if (!xsltName().empty() && server && !server->needApplyPerblockStylesheet(ctx->request())) {
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
TaggedBlock::postParse() {
    if (tagged()) {
        if ((CACHE_TIME_UNDEFINED != cache_time_) && (cache_time_ < DocCache::instance()->minimalCacheTime())) {
            tagged(false);
        }
    }
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

} // namespace xscript
