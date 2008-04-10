#include <boost/bind.hpp>
#include "settings.h"
#include "xscript/tagged_cache.h"
#include "xscript/control_extension.h"
#include "xscript/profiler.h"
#include "xscript/tag.h"

#include "details/dummy_tagged_cache.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

TagKey::TagKey()
{
}

TagKey::~TagKey() {
}

TaggedCache::TaggedCache()
	: statBuilder_("tagged-cache"), hitCounter_("hits"), missCounter_("miss")
{
	statBuilder_.addCounter(hitCounter_);
	statBuilder_.addCounter(missCounter_);
}

TaggedCache::~TaggedCache() {
}

void TaggedCache::init(const Config *config) {
	(void)config;

	ControlExtensionRegistry::constructor_t f = boost::bind(boost::mem_fn(&StatBuilder::createBlock), &statBuilder_, _1, _2, _3);

    ControlExtensionRegistry::registerConstructor("tagged-cache-stat", f);
}



bool TaggedCache::loadDoc(const TagKey *key, Tag &tag, XmlDocHelper &doc) {

	//return loadDocImpl(key, tag, doc);
	
	boost::function<bool ()> f = boost::bind(&TaggedCache::loadDocImpl, this, boost::cref(key), boost::ref(tag), boost::ref(doc));
	std::pair<bool, uint64_t> res = profile(f);

	if (res.first) 
		hitCounter_.add(res.second);
	else
		missCounter_.add(res.second);
	return res.first;
}

bool TaggedCache::saveDoc(const TagKey *key, const Tag& tag, const XmlDocHelper &doc) {
	return saveDocImpl(key, tag, doc);
}

REGISTER_COMPONENT2(TaggedCache, DummyTaggedCache);

} // namespace xscript
