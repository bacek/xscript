#include "settings.h"

#include <boost/current_function.hpp>

#include "xscript/logger.h"
#include "xscript/tagged_cache.h"
#include "xscript/tagged_block.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

static const time_t CACHE_TIME_MINIMUM = 5;

TaggedBlock::TaggedBlock(Xml *owner, xmlNodePtr node) :
	Block(owner, node), canonical_method_(), tagged_(false), cache_time_(Tag::UNDEFINED_TIME)
{
}

TaggedBlock::~TaggedBlock() {
}

std::string
TaggedBlock::canonicalMethod(const Context *ctx) const {
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

void
TaggedBlock::cacheTime(time_t cache_time) {
	log()->debug("%s, cache_time: %u", BOOST_CURRENT_FUNCTION, cache_time);
	cache_time_ = cache_time;
}

XmlDocHelper
TaggedBlock::invoke(Context *ctx) {
	
	log()->debug("%s", BOOST_CURRENT_FUNCTION);

	if (!tagged()) {
		return Block::invoke(ctx);
	}
	
	try {
		Tag tag;
		XmlDocHelper doc(NULL);
		
		TaggedCache *cache = TaggedCache::instance();
		std::auto_ptr<TagKey> key = cache->createKey(ctx, this);
		
		if (!cache->loadDoc(key.get(), tag, doc)) {
			return Block::invoke(ctx);
		}

		if (Tag::UNDEFINED_TIME == tag.expire_time) {
			
			tag.modified = true;
			
			boost::any a(tag);
			XmlDocHelper newdoc = call(ctx, a);
			tag = boost::any_cast<Tag>(a);
			
			if (NULL != newdoc.get()) {
				postCall(ctx, newdoc, a);
				evalXPath(ctx, newdoc);
				return newdoc;
			}
			else if (tag.modified) {
				return Block::invoke(ctx);
			}
		}
		evalXPath(ctx, doc);
		return doc;
	}
	catch (const std::exception &e) {
		log()->error("caught exception while invoking tagged block: %s", e.what());
	}
	return Block::invoke(ctx);
}

void
TaggedBlock::postCall(Context *ctx, const XmlDocHelper &doc, const boost::any &a) {

	log()->debug("%s, tagged: %d", BOOST_CURRENT_FUNCTION, static_cast<int>(tagged()));
	if (!tagged()) {
		return;
	}
	
	time_t now = time(NULL);
	Tag tag = boost::any_cast<Tag>(a);
	TaggedCache *cache = TaggedCache::instance();
	
	bool can_store = false;
	if (Tag::UNDEFINED_TIME != cache_time_) {
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
		std::auto_ptr<TagKey> key = cache->createKey(ctx, this);
		cache->saveDoc(key.get(), tag, doc);
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
